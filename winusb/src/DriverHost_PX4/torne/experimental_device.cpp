// SPDX-License-Identifier: GPL-2.0-only
#include "experimental_device.hpp"
#include "device_identity.hpp"
#ifdef PX4_TORNE_EXPERIMENTAL
#include "frontend_client.hpp"
#else
#include "frontend_native_client.hpp"
#endif
#include "winusb_control.hpp"
#include "dib8096_registers.hpp"
#include "mb86h47_stream.hpp"
#include "stream_framer.hpp"
#include "ts_quality.hpp"
#include "tune_result.hpp"
#include "mb86h47_card.hpp"
#include "../../WinSCard_PX4/bcas_atr.hpp"
#include "msg.h"
#include "util.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace px4::torne {
using Clock = std::chrono::steady_clock;
namespace {
struct Event {
	HANDLE value = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	~Event() { if (value) CloseHandle(value); }
	Event() = default;
	Event(const Event &) = delete;
	Event &operator=(const Event &) = delete;
};

// TS用bulk INの1転送。投入中に破棄する場合は取消して完了を回収し、
// bufferとOVERLAPPEDの寿命がWinUSBの処理より先に終わらないようにする。
struct StreamTransfer {
	StreamTransfer(HANDLE file_handle, WINUSB_INTERFACE_HANDLE usb_handle, std::size_t size)
		: file(file_handle), usb(usb_handle), buffer(size) { operation.hEvent = complete.value; }
	~StreamTransfer() { Collect(); }
	StreamTransfer(const StreamTransfer &) = delete;
	StreamTransfer &operator=(const StreamTransfer &) = delete;
	// 投入に失敗した場合はWindowsのエラーを返す。
	DWORD Submit()
	{
		if (!ResetEvent(complete.value)) return GetLastError();
		operation.Internal = operation.InternalHigh = 0;
		operation.Offset = operation.OffsetHigh = 0;
		if (!WinUsb_ReadPipe(usb, 0x81, buffer.data(), static_cast<ULONG>(buffer.size()), nullptr, &operation)) {
			const DWORD error = GetLastError();
			if (error != ERROR_IO_PENDING) return error;
		}
		active = true;
		return 0;
	}
	void Collect()
	{
		if (!active) return;
		CancelIoEx(file, &operation);
		ULONG ignored = 0;
		WinUsb_GetOverlappedResult(usb, &operation, &ignored, TRUE);
		active = false;
	}
	HANDLE file;
	WINUSB_INTERFACE_HANDLE usb;
	Event complete;
	std::vector<std::uint8_t> buffer;
	OVERLAPPED operation{};
	bool active = false;
};

} // namespace

// 制御mutexはhelper経由のI2Cとworkerのbridge操作の両方を覆う。
// TS readの待機はこのmutexを保持せず、統計取得による受信停止を避ける。
struct ExperimentalSession {
	ExperimentalSession(HANDLE file_handle, WINUSB_INTERFACE_HANDLE usb_handle, HANDLE removed)
		: file(file_handle), usb(usb_handle), cancel(removed), control(file, usb, cancel, native),
		memory(control), bootstrap(memory, control), bus(memory, control),
		registers(bus), frontend(bus, cancel), engine(memory, bootstrap) {}
	bool Command(std::uint8_t command, const std::vector<std::uint8_t> &body,
		std::vector<std::uint8_t> &response, Clock::time_point deadline)
	{
		std::lock_guard<std::mutex> lock(control_mutex);
		if (!healthy || !frontend_started) return false;
		control.Begin(deadline);
		const bool ok = frontend.Command(command, body, response, deadline);
		if (!ok) { healthy = false; bus.Invalidate(); }
		return ok;
	}
	void Invalidate()
	{
		std::lock_guard<std::mutex> lock(control_mutex);
		healthy = false;
		bus.Invalidate();
	}
	bool InitializeBridge()
	{
		control.Begin(Clock::now() + std::chrono::seconds(8));
		return bootstrap.Initialize(GetTickCount() | 1u) == BootstrapResult::ok && bus.BeginAfterBootstrap(bootstrap);
	}
	bool InitializeFrontend(bool output_error_packets)
	{
		std::lock_guard<std::mutex> lock(control_mutex);
		if (!healthy) return false;
		const auto deadline = Clock::now() + std::chrono::seconds(8);
		control.Begin(deadline);
#ifndef PX4_TORNE_EXPERIMENTAL
        // 正常な同一個体だけ再開する。失敗したセッションを成功扱いで使い回さない。
        if(frontend_started){
            std::vector<std::uint8_t> response;
            const bool ok=frontend.Command(0x15,{},response,deadline) && response.empty();
            if(!ok){healthy=false;bus.Invalidate();}
            return ok;
        }
#endif
		if (bootstrap.ResetDemodulator() != BootstrapResult::ok) return false;
		if (registers.Write(1287, 3, deadline) != I2cResult::ok) return false;
		std::uint16_t vendor = 0, chip = 0;
		bool recognized = false;
		if (registers.Identify(vendor, chip, recognized, deadline) != I2cResult::ok || !recognized) return false;
#ifdef PX4_TORNE_EXPERIMENTAL
		const auto helper = px4::util::path::GetDir() + L"TorneFrontendExperimental.exe";
		if (!frontend_started && !frontend.Start(helper)) return false;
#else
        frontend.ConfigureTuning(true,true,true);
#endif
		frontend_started = true;
		std::vector<std::uint8_t> response;
		bool ok = frontend.Command(0x10, {}, response, deadline) && response.empty();
		if (ok && output_error_packets) {
			// SMO error-discardを解除する診断用設定。受信できない層の復元や
			// ホスト側のnull補完は行わず、復調器が出したTEIもそのまま上位へ渡す。
			std::uint16_t mode=0,verified=0;
			ok=registers.Read(299,mode,deadline)==I2cResult::ok &&
				registers.Write(299,static_cast<std::uint16_t>(mode & ~0x0040u),deadline)==I2cResult::ok &&
				registers.Read(299,verified,deadline)==I2cResult::ok && !(verified & 0x0040);
			msg_info("torne: OutputErrorPackets SMO=%04x verified=%04x result=%d\n",mode,verified,ok);
		}
		if (!ok) { healthy = false; bus.Invalidate(); }
		return ok;
	}
	HANDLE file;
	WINUSB_INTERFACE_HANDLE usb;
	HANDLE cancel;
	NativeControlIo native;
	WinUsbControl control;
	MemoryAccess memory;
	Bootstrap bootstrap;
	MailboxI2c bus;
	DiB8096Registers registers;
#ifdef PX4_TORNE_EXPERIMENTAL
	TorneFrontendClient frontend;
#else
	NativeFrontendClient frontend;
#endif
	StreamEngine engine;
	std::mutex control_mutex;
	bool healthy = true, frontend_started = false;
};

struct ExperimentalDevice::CardState {
	explicit CardState(std::shared_ptr<ExperimentalSession> owner) : session(std::move(owner)) {}
	std::shared_ptr<ExperimentalSession> session;
	std::vector<std::uint8_t> received;
	std::size_t remaining = 0;
	bool atr = true;
};

int ExperimentalDevice::OpenCard() try
{
	std::lock_guard<std::mutex> lock(card_mutex_);
	if (card_) return -EBUSY;
	auto session = AcquireSession();
	if (!session) return -ENODEV;
	card_ = std::make_unique<CardState>(std::move(session));
	return 0;
} catch (...) { return -ENOMEM; }

void ExperimentalDevice::CloseCard()
{
	std::lock_guard<std::mutex> lock(card_mutex_);
	if (!card_) return;
	if (available_) {
		std::lock_guard<std::mutex> control_lock(card_->session->control_mutex);
		card_->session->control.Begin(Clock::now() + std::chrono::seconds(1));
		CardUart uart(card_->session->memory, card_->session->control);
		if (!uart.PowerDown()) msg_warn("torne: card power down failed\n");
	}
	card_.reset();
}

int ExperimentalDevice::DetectCard(bool &detected)
{
	std::lock_guard<std::mutex> lock(card_mutex_);
	if (!card_ || !available_) return -ENODEV;
	std::lock_guard<std::mutex> control_lock(card_->session->control_mutex);
	card_->session->control.Begin(Clock::now() + std::chrono::seconds(1));
	CardUart uart(card_->session->memory, card_->session->control);
	if (!uart.Present(detected)) return -EIO;
	if (!detected) { card_->received.clear(); card_->remaining = 0; }
	return 0;
}

int ExperimentalDevice::ResetCard()
{
	std::lock_guard<std::mutex> lock(card_mutex_);
	if (!card_ || !available_) return -ENODEV;
	card_->received.clear(); card_->remaining = 0; card_->atr = true;
	std::lock_guard<std::mutex> control_lock(card_->session->control_mutex);
	card_->session->control.Begin(Clock::now() + std::chrono::seconds(1));
	CardUart uart(card_->session->memory, card_->session->control);
	return uart.Reset() ? 0 : -EIO;
}

int ExperimentalDevice::SetCardBaudrate(::it930x_uart_baudrate baudrate)
{
	// この試験段階は実機でATRを確認したB-CAS/LRCだけを対象にする。
	if (baudrate != IT930X_UART_BAUDRATE_19200) return -ENOTSUP;
	std::lock_guard<std::mutex> lock(card_mutex_);
	if (!card_ || !available_) return -ENODEV;
	std::lock_guard<std::mutex> control_lock(card_->session->control_mutex);
	card_->session->control.Begin(Clock::now() + std::chrono::seconds(1));
	CardUart uart(card_->session->memory, card_->session->control);
	return uart.SetBaudrate(19200) ? 0 : -EIO;
}

int ExperimentalDevice::IsCardDataReady(bool &ready) try
{
	std::lock_guard<std::mutex> lock(card_mutex_);
	ready = false;
	if (!card_ || !available_) return -ENODEV;
	if (card_->remaining) { ready = true; return 0; }
	std::array<std::uint8_t,259> bytes{};
	std::size_t count = 0;
	{
		std::lock_guard<std::mutex> control_lock(card_->session->control_mutex);
		card_->session->control.Begin(Clock::now() + std::chrono::seconds(1));
		CardUart uart(card_->session->memory, card_->session->control);
		if (!uart.ReadBytes(bytes.data(), bytes.size(), count)) return -EIO;
	}
	if (card_->received.size() + count > 518) return -EMSGSIZE;
	card_->received.insert(card_->received.end(), bytes.begin(), bytes.begin() + count);
	if (card_->atr) {
		if (px4::winscard::IsBcasAtr(card_->received.data(), card_->received.size())) card_->remaining = card_->received.size();
		else if (card_->received.size() >= 33) return -ENOTSUP;
	} else if (card_->received.size() >= 3) {
		const auto length = std::size_t(4) + card_->received[2];
		if (card_->received.size() >= length) card_->remaining = length;
	}
	ready = card_->remaining != 0;
	return 0;
} catch (...) { return -ENOMEM; }

int ExperimentalDevice::ReadCardData(std::uint8_t *buf, std::uint8_t &len)
{
	std::lock_guard<std::mutex> lock(card_mutex_);
	if (!card_ || !available_) return -ENODEV;
	if (!buf || !len) return -EINVAL;
	const auto count = (std::min)(std::size_t(len), card_->remaining);
	std::copy_n(card_->received.begin(), count, buf);
	card_->received.erase(card_->received.begin(), card_->received.begin() + count);
	card_->remaining -= count;
	if (count && card_->atr && !card_->remaining) card_->atr = false;
	len = static_cast<std::uint8_t>(count);
	return 0;
}

int ExperimentalDevice::WriteCardData(const std::uint8_t *buf, std::uint8_t len)
{
	std::lock_guard<std::mutex> lock(card_mutex_);
	if (!card_ || !available_) return -ENODEV;
	std::lock_guard<std::mutex> control_lock(card_->session->control_mutex);
	card_->session->control.Begin(Clock::now() + std::chrono::seconds(1));
	CardUart uart(card_->session->memory, card_->session->control);
	return uart.WriteBytes(buf, len) ? 0 : -EIO;
}

std::shared_ptr<ExperimentalSession> ExperimentalDevice::AcquireSession()
{
	// 個体単位のbridgeを共有し、受信とカードがそれぞれbootstrapしない。
	std::lock_guard<std::mutex> lock(session_mutex_);
	if (!available_) return {};
	if (auto current = shared_session_.lock()) {
		bool healthy = false;
		{ std::lock_guard<std::mutex> control(current->control_mutex); healthy = current->healthy; }
		if (healthy) return current;
		// 通信に失敗したセッションは以後の命令をすべて拒否する。保持したままだと
		// TVTestを閉じても同じセッションが再利用され、認識はしているのに選局できなくなる。
		// 保持をやめ、他(カード)が使っていなければbridgeから初期化し直す。
		// まだ使われている間は同じ個体のbridgeを二重に初期化しないよう、作り直さない。
#ifndef PX4_TORNE_EXPERIMENTAL
		if (retained_session_ == current) retained_session_.reset();
#endif
		if (current.use_count() > 1) return {};
	}
	auto created = std::make_shared<ExperimentalSession>(usb_dev_.dev, usb_dev_.winusb, cancel_);
	if (!created->InitializeBridge() || !available_) return {};
	shared_session_ = created;
#ifndef PX4_TORNE_EXPERIMENTAL
    retained_session_=created;
#endif
	return created;
}

class ExperimentalDevice::Receiver final : public px4::ReceiverBase {
public:
	explicit Receiver(ExperimentalDevice &parent) : ReceiverBase(0), parent_(parent) {}
	~Receiver() override { Close(); }
	int Open() override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (open_) return -EBUSY;
		if (!parent_.available_ || !stop_.value) return -ENODEV;
		if (!parent_.available_) { SetEvent(parent_.cancel_); return -ENODEV; }
		try {
			session_ = parent_.AcquireSession();
			if (!session_) return -EIO;
			if (!session_->InitializeFrontend(parent_.output_error_packets_)) {
				msg_err("torne: initialize failed (%s).\n", session_->frontend.Error().c_str());
				SleepFrontend();
				session_.reset();
				return -EIO;
			}
			if (!stream_buf_->Alloc(188u * 65536u)) { SleepFrontend(); session_.reset(); return -ENOMEM; }
		} catch (...) { session_.reset(); return -ENOMEM; }
		stream_buf_->SetThresholdSize(188u * 16u);
		stream_buf_->Start();
		capture_ = tuned_ = failed_ = false;
		open_ = true;
		msg_info("torne: open id=%s\n", parent_.id_.c_str());
		return 0;
	}
	void Close() override
	{
		std::unique_lock<std::mutex> lock(mutex_);
		if (!open_) return;
		StopReader();
		SleepFrontend();
		session_.reset();
		stream_buf_->Stop();
		capture_ = tuned_ = failed_ = open_ = false;
		lock.unlock();
		closed_.notify_all();
	}
	void WaitClosed()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		closed_.wait(lock, [this] { return !open_; });
	}
	int SetCapture(bool capture) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!open_ || !parent_.available_) return -ENODEV;
		capture_ = capture;
		if (!capture) return StopReader();
		if (failed_) return -EIO;
		return tuned_ ? StartReader(Clock::now() + std::chrono::seconds(2)) : 0;
	}
	int SetLnbVoltage(std::int32_t voltage) override { return voltage ? -EINVAL : 0; }
	int CheckLock(bool &locked) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		locked = false;
		if (!open_ || !parent_.available_) return -ECANCELED;
		if (failed_) return -ECANCELED;
		std::vector<std::uint8_t> response;
		if (!session_->Command(0x13, {}, response, Clock::now() + std::chrono::seconds(1)) || response.size() != 24) return -ECANCELED;
		locked = tuned_ && response[0] == 1;
		if (capture_) {
			std::lock_guard<std::mutex> state(state_mutex_);
			locked = locked && first_ && !finished_;
		}
		return 0;
	}
	int ReadStat(px4::command::StatType type, std::int32_t &value) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		value = 0;
		if (!open_ || !parent_.available_) return -ENODEV;
		std::vector<std::uint8_t> response;
		if (!session_->Command(0x13, {}, response, Clock::now() + std::chrono::seconds(1)) || response.size() != 24) return -EIO;
		double reading = 0;
		std::size_t offset = 0;
		if (type == px4::command::StatType::CNR && response[1]) offset = 8;
		else if (type == px4::command::StatType::SIGNAL_STRENGTH && response[2]) offset = 16;
		else return -ENOSYS;
		std::memcpy(&reading, response.data() + offset, sizeof(reading));
		if (!std::isfinite(reading) || reading < -200.0 || reading > 200.0) return -EIO;
		value = static_cast<std::int32_t>(std::lround(reading * 1000.0));
		return 0;
	}
protected:
	int SetStreamId() override { return 0; }
	int SetFrequency() override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!open_ || !parent_.available_) return -ENODEV;
		if (params_.system != px4::SystemType::ISDB_T || params_.freq < 473143 || params_.freq > 767143 ||
			(params_.bandwidth && params_.bandwidth != 6 && params_.bandwidth != 6000 && params_.bandwidth != 6000000)) return -EINVAL;
		// 待ち時間の短縮は、停止・復調器・ロック確認・先頭TSを分けて実測して判断する。
		const auto tune_begin = Clock::now();
		auto elapsed_ms = [](Clock::time_point begin, Clock::time_point end) {
			return static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
		};
		tuned_ = false;
		if (StopReader()) { failed_ = true; return -EIO; }
		if (!stream_buf_->Purge()) return -EIO;
		stream_buf_->ResetInputPacketCount();
		failed_ = false;
		// 実験版は復調器/lock/先頭TSを共通の5秒予算へ含める。旧機種のTuneは変更しない。
		const auto deadline = Clock::now() + std::chrono::seconds(5);
		const auto hz = params_.freq * 1000u;
		std::vector<std::uint8_t> frequency(4), response;
		for (unsigned i = 0; i < 4; ++i) frequency[i] = static_cast<std::uint8_t>(hz >> (8 * i));
		const auto demod_begin = Clock::now();
		if (!session_->Command(0x11, frequency, response, deadline)) { failed_ = true; return -EIO; }
		const auto demod_end = Clock::now();
		if (IsConfirmedSearchFailure(response.data(), response.size())) {
			failed_ = true;
			msg_info("torne: confirmed search failure id=%s freq_khz=%u\n", parent_.id_.c_str(), params_.freq);
			return -ECANCELED;
		}
		bool locked = false;
		while (Clock::now() < deadline) {
			if (!session_->Command(0x13, {}, response, deadline) || response.size() != 24) { failed_ = true; return -EIO; }
			if (response[0] == 1) { locked = true; break; }
			if (WaitForSingleObject(parent_.cancel_, 20) != WAIT_TIMEOUT) return -ECANCELED;
		}
		if (!locked) { failed_ = true; return -ETIMEDOUT; }
		const auto lock_end = Clock::now();
		tuned_ = true;
		const int result = capture_ ? StartReader(deadline) : 0;
		const auto stream_end = Clock::now();
		if (result) failed_ = true;
		msg_info("torne: tune id=%s freq_khz=%u result=%d capture=%d stop_ms=%lld demod_ms=%lld lock_check_ms=%lld stream_ready_ms=%lld total_ms=%lld\n",
			parent_.id_.c_str(), params_.freq, result, capture_,
			elapsed_ms(tune_begin, demod_begin), elapsed_ms(demod_begin, demod_end),
			elapsed_ms(demod_end, lock_end), elapsed_ms(lock_end, stream_end), elapsed_ms(tune_begin, stream_end));
		return result;
	}
private:
	void SleepFrontend()
	{
		if (!session_ || !parent_.available_) return;
		std::vector<std::uint8_t> response;
		if (!session_->Command(0x14, {}, response, Clock::now() + std::chrono::seconds(2))) msg_warn("torne: sleep/LED release failed\n");
	}
	int StartReader(Clock::time_point deadline)
	{
		if (worker_.joinable()) { std::lock_guard<std::mutex> state(state_mutex_); return finished_ ? -EIO : 0; }
		std::vector<std::uint8_t> response;
		if (!session_->Command(0x12, {1}, response, deadline) || !ResetEvent(stop_.value)) return -EIO;
		{ std::lock_guard<std::mutex> state(state_mutex_); first_ = finished_ = false; }
		worker_error_ = 0;
		worker_stop_ok_ = false;
		try { worker_ = std::thread([this, deadline] { ReadLoop(deadline); }); }
		catch (...) { failed_ = true; return -ENOMEM; }
		std::unique_lock<std::mutex> state(state_mutex_);
		const auto first_deadline = (std::min)(deadline, Clock::now() + std::chrono::seconds(2));
		const bool ready = changed_.wait_until(state, first_deadline, [this] { return first_ || finished_; });
		const bool good = ready && first_ && !finished_;
		state.unlock();
		if (!good) { StopReader(); failed_ = true; return -ETIMEDOUT; }
		return 0;
	}
	void ReadLoop(Clock::time_point start_deadline) noexcept
	{
		// 他のPX系の受信スレッドと同じ優先度にする。通常優先度ではブラウザーなどの
		// 負荷で完了処理と再投入が遅れ、bridgeのFIFOが溢れてTSが欠落する。
		if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
			msg_warn("torne: SetThreadPriority(TIME_CRITICAL) failed (%lu)\n", GetLastError());
		StreamFramer framer(parent_.output_error_packets_);
		TsQuality quality;
		std::vector<std::unique_ptr<StreamTransfer>> transfers;
		std::vector<std::uint8_t> output;
		std::uint64_t overflow_packets = 0, overflow_events = 0;
		bool prepared = false, running = false;
		try {
			{
				std::lock_guard<std::mutex> lock(session_->control_mutex);
				session_->control.Begin(start_deadline);
				prepared = session_->engine.Prepare() == StreamResult::ok;
			}
			if (!prepared || !WinUsb_ResetPipe(parent_.usb_dev_.winusb, 0x81)) worker_error_ = ERROR_GEN_FAILURE;
			// 1本だけの転送では、完了から再投入までの間にbridgeから受け取る先がなくなる。
			// 複数を常に投入しておき、受信スレッドが一時的に遅れても取りこぼさないようにする。
			for (unsigned int i = 0; i < parent_.usb_transfer_count_ && !worker_error_; ++i) {
				auto transfer = std::make_unique<StreamTransfer>(parent_.usb_dev_.dev, parent_.usb_dev_.winusb, parent_.usb_transfer_size_);
				if (!transfer->complete.value) { worker_error_ = ERROR_NOT_ENOUGH_MEMORY; break; }
				if (const DWORD error = transfer->Submit()) { worker_error_ = error; break; }
				transfers.push_back(std::move(transfer));
			}
			// bridgeの出力開始は受け取り先の転送を投入した後に行う。
			if (!worker_error_) {
				std::lock_guard<std::mutex> lock(session_->control_mutex);
				session_->control.Begin(start_deadline);
				running = session_->engine.Start() == StreamResult::ok;
				if (!running) worker_error_ = ERROR_GEN_FAILURE;
			}
			// 投入順に完了を回収し、TSの順序を保つ。
			std::size_t next = 0;
			while (!worker_error_) {
				if (WaitForSingleObject(stop_.value, 0) == WAIT_OBJECT_0 || !parent_.available_) break;
				auto &transfer = *transfers[next];
				const HANDLE events[]{stop_.value, parent_.cancel_, transfer.complete.value};
				const DWORD waited = WaitForMultipleObjects(3, events, FALSE, 2000);
				if (waited != WAIT_OBJECT_0 + 2) {
					worker_error_ = waited == WAIT_TIMEOUT ? ERROR_TIMEOUT : waited == WAIT_FAILED ? GetLastError() : ERROR_OPERATION_ABORTED;
					break;
				}
				ULONG transferred = 0;
				const BOOL collected = WinUsb_GetOverlappedResult(parent_.usb_dev_.winusb, &transfer.operation, &transferred, TRUE);
				transfer.active = false;
				if (!collected) { worker_error_ = GetLastError(); break; }
				if (transferred > transfer.buffer.size()) { worker_error_ = ERROR_INVALID_DATA; break; }
				output.clear();
				framer.Feed(transfer.buffer.data(), transferred, output);
				// 内容を取り出した後すぐに再投入し、TS配信の処理時間を転送の空白にしない。
				if (const DWORD error = transfer.Submit()) worker_error_ = error;
				bool overflowed = false;
				for (std::size_t offset = 0; offset < output.size(); offset += 188) {
					quality.Feed(output.data() + offset);
					if (overflowed) { ++overflow_packets; continue; }
					std::size_t count = 188;
					if (!stream_buf_->Write(output.data() + offset, count) || count != 188) {
						// 利用側が読み出しを止めている間は書けない分を捨てて受信を続ける。
						// 他のPX系と同じ扱いで、受信を止めると録画が復帰しなくなる。
						overflowed = true;
						++overflow_packets;
					}
				}
				if (overflowed && (overflow_events++ % 64) == 0)
					msg_warn("torne: stream buffer full id=%s dropped_packets=%llu\n", parent_.id_.c_str(), static_cast<unsigned long long>(overflow_packets));
				stream_buf_->NotifyWrite();
				if (!output.empty()) {
					{ std::lock_guard<std::mutex> state(state_mutex_); first_ = true; }
					changed_.notify_all();
				}
				next = (next + 1) % transfers.size();
			}
		} catch (...) { worker_error_ = ERROR_NOT_ENOUGH_MEMORY; }
		// readのOVERLAPPEDをすべて回収した後だけbridgeを停止し、次の選局とbuffer再利用を許可する。
		for (auto &transfer : transfers) transfer->Collect();
		transfers.clear();
		{
			std::lock_guard<std::mutex> lock(session_->control_mutex);
			session_->control.Begin(Clock::now() + std::chrono::seconds(2));
			worker_stop_ok_ = session_->engine.Stop() == StreamResult::ok;
			if (!worker_stop_ok_ && !worker_error_) worker_error_ = ERROR_GEN_FAILURE;
		}
		{ std::lock_guard<std::mutex> state(state_mutex_); finished_ = true; }
		changed_.notify_all();
		msg_info("torne: reader end id=%s error=%lu stop_ok=%d buffer_full_dropped=%llu packets=%llu tei=%llu discarded=%llu raw_cc=%llu raw_malformed=%llu scrambled=%llu\n",
			parent_.id_.c_str(), worker_error_, worker_stop_ok_, static_cast<unsigned long long>(overflow_packets),
			static_cast<unsigned long long>(framer.Packets()),
			static_cast<unsigned long long>(framer.TeiPackets()), static_cast<unsigned long long>(framer.DiscardedBytes()),
			static_cast<unsigned long long>(quality.cc_anomalies), static_cast<unsigned long long>(quality.malformed), static_cast<unsigned long long>(quality.scrambled));
	}
	int StopReader()
	{
		if (!worker_.joinable()) return 0;
		SetEvent(stop_.value);
		worker_.join();
		std::vector<std::uint8_t> response;
		if (!worker_stop_ok_) session_->Invalidate();
		const bool parked = !parent_.available_ || (worker_stop_ok_ && session_->Command(0x12, {0}, response, Clock::now() + std::chrono::seconds(1)));
		// 受信の成否と停止状態を分ける。停止確認後なら次の明示的選局で復帰を試せる。
		const bool good = parked && worker_stop_ok_;
		if (!good || (worker_error_ && worker_error_ != ERROR_OPERATION_ABORTED)) msg_warn("torne: receive stopped with error=%lu known_state=%d\n", worker_error_, good);
		return good ? 0 : -EIO;
	}
	ExperimentalDevice &parent_;
	std::mutex mutex_, state_mutex_;
	std::condition_variable changed_, closed_;
	bool open_ = false, capture_ = false, tuned_ = false, failed_ = false;
	bool first_ = false, finished_ = false;
	Event stop_;
	std::shared_ptr<ExperimentalSession> session_;
	std::thread worker_;
	DWORD worker_error_ = 0; // workerだけが書き、join後に制御側が読む
	bool worker_stop_ok_ = false;
};

ExperimentalDevice::ExperimentalDevice(const std::wstring &path, const px4::DeviceDefinition &definition,
	std::uintptr_t index, px4::ReceiverManager &manager)
	: DeviceBase(path, definition, index, manager), id_(InterfaceId(path))
{
	cancel_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!cancel_) throw DeviceError("torne: CreateEvent failed");
}
ExperimentalDevice::~ExperimentalDevice() { Term(); if (cancel_) CloseHandle(cancel_); }
int ExperimentalDevice::Init()
{
	output_error_packets_=px4::util::wtob(device_def_.configs.Get(L"OutputErrorPackets",L"false"));
	// TS受信のUSB転送は他のPX系と同じUrbMaxPackets(1転送のTSパケット数)とMaxUrbs(同時投入数)で指定する。
	// UrbMaxPacketsが無ければXferPacketsを使う。torneのbridgeには別の転送単位が無いため、どちらも1転送の長さになる。
	// bulkの最大パケット長512の倍数でないと転送の途中で短いパケットとして完了するため、512の倍数へ切り下げる。
	// TSは転送の境界をまたいでもStreamFramerで組み直すので、188の倍数である必要はない。
	// 旧名のUsbTransferSize(バイト数)とUsbTransferCountが書かれていれば、移行期間はそちらを優先する。
	{
		const auto &configs=device_def_.configs;
		constexpr std::uint32_t default_size=6144, default_count=16, max_size=1048576;
		std::uint32_t size=default_size;
		if (configs.Exists(L"UsbTransferSize")) {
			size=px4::util::wtoui32(configs.Get(L"UsbTransferSize"));
		} else if (configs.Exists(L"UrbMaxPackets") || configs.Exists(L"XferPackets")) {
			const auto packets=px4::util::wtoui32(configs.Get(configs.Exists(L"UrbMaxPackets")?L"UrbMaxPackets":L"XferPackets"));
			size=(packets>0 && packets<=max_size/188)?packets*188/512*512:0;
		}
		usb_transfer_size_=(size>=512 && size<=max_size && size%512==0)?size:default_size;
		std::uint32_t count=default_count;
		if (configs.Exists(L"UsbTransferCount")) count=px4::util::wtoui32(configs.Get(L"UsbTransferCount"));
		else if (configs.Exists(L"MaxUrbs")) count=px4::util::wtoui32(configs.Get(L"MaxUrbs"));
		usb_transfer_count_=(count>=1 && count<=64)?count:default_count;
	}
	msg_info("torne: config usb_xfer=%u x %u\n",usb_transfer_size_,usb_transfer_count_);
	// 接続個体をすべて登録し、BonDriverのReceiverNameで利用個体を選ぶ。
	if (usb_dev_.descriptor.idVendor != 0x054c || usb_dev_.descriptor.idProduct != 0x04b2 || device_def_.receivers.size() != 1) return -EINVAL;
	UCHAR alternate = 0xff;
	USB_INTERFACE_DESCRIPTOR descriptor{};
	if (!WinUsb_GetCurrentAlternateSetting(usb_dev_.winusb, &alternate) || alternate != 0 ||
		!WinUsb_QueryInterfaceSettings(usb_dev_.winusb, 0, &descriptor) || descriptor.bInterfaceNumber != 0) return -EINVAL;
	bool stream = false, command = false, answer = false;
	for (UCHAR i = 0; i < descriptor.bNumEndpoints; ++i) {
		WINUSB_PIPE_INFORMATION pipe{};
		if (!WinUsb_QueryPipe(usb_dev_.winusb, 0, i, &pipe)) return -EIO;
		stream |= pipe.PipeId == 0x81 && pipe.PipeType == UsbdPipeTypeBulk && pipe.MaximumPacketSize == 512;
		command |= pipe.PipeId == 0x02 && pipe.PipeType == UsbdPipeTypeBulk;
		answer |= pipe.PipeId == 0x83 && pipe.PipeType == UsbdPipeTypeInterrupt;
	}
	if (!stream || !command || !answer) return -EINVAL;
	receiver_ = std::make_unique<Receiver>(*this);
	px4::command::ReceiverInfo info{};
	const auto &definition = device_def_.receivers.front();
	wcscpy_s(info.device_name, device_def_.name.c_str());
	info.device_guid = device_def_.guid;
	const auto name = definition.name + L" [" + std::wstring(id_.begin(), id_.end()) + L"]";
	wcscpy_s(info.receiver_name, name.c_str());
	info.receiver_guid = definition.guid;
	info.systems = px4::SystemType::ISDB_T;
	info.index = definition.index;
	registered_ = receiver_manager_.Register(info, receiver_.get());
	return registered_ ? 0 : -EIO;
}
void ExperimentalDevice::Term()
{
	SetAvailability(false);
	if (registered_) { receiver_manager_.Unregister(receiver_.get()); registered_ = false; }
	if (receiver_) { receiver_->WaitClosed(); receiver_.reset(); }
    // 抜去後のセッションを次のUSB個体へ持ち越さない。
    {std::lock_guard<std::mutex> lock(session_mutex_);retained_session_.reset();}
}
void ExperimentalDevice::SetAvailability(bool available)
{
	available_ = available;
	if (!available && cancel_) SetEvent(cancel_);
}
px4::ReceiverBase *ExperimentalDevice::GetReceiver(int id) const
{
	if (id != 0) throw std::out_of_range("torne receiver index");
	return receiver_.get();
}
} // namespace px4::torne
