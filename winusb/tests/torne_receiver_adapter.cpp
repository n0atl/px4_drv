// SPDX-License-Identifier: GPL-2.0-only
#include "torne_receiver_adapter.hpp"
#include "../src/DriverHost_PX4/receiver_base.hpp"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <thread>

namespace {
using Clock = std::chrono::steady_clock;

// 診断専用。呼出側は単一制御スレッド、受信workerはTS読み出しだけを所有する。
// helper/USB/制御deadlineは外側の試験が所有し、このオブジェクトより長生きする。
class Receiver final : public px4::ReceiverBase {
public:
	Receiver(HANDLE file, WINUSB_INTERFACE_HANDLE usb, px4::torne::MemoryAccess &memory,
		px4::torne::Bootstrap &bootstrap, TorneReferenceClient &reference, Clock::time_point deadline)
		: ReceiverBase(0), file_(file), usb_(usb), engine_(memory, bootstrap), reference_(reference), deadline_(deadline)
	{
		cancel_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	}
	~Receiver() override { Close(); if (cancel_) CloseHandle(cancel_); }
	int Open() override
	{
		if (open_) return -EBUSY;
		if (!cancel_ || !stream_buf_->Alloc(188u * 65536u)) return -ENOMEM;
		open_ = true;
		stream_buf_->SetThresholdSize(188u * 16u);
		stream_buf_->Start();
		return 0;
	}
	void Close() override
	{
		if (!open_) return;
		SetCapture(false);
		stream_buf_->Stop();
		open_ = tuned_ = false;
		requested_ = capture_failed_ = false;
	}
	int SetCapture(bool capture) override
	{
		if (!open_) return -EINVAL;
		requested_ = capture;
		if (!capture) return Stop();
		if (capture_failed_) return -ECANCELED;
		// BonDriverのOpen時点には周波数がない。要求だけを保持しUSB転送を始めない。
		return tuned_ ? Start() : 0;
	}
	int CheckLock(bool &locked) override
	{
		locked = false;
		if (!open_ || !tuned_) return -EINVAL;
		if (capture_failed_) return -ECANCELED;
		if (worker_.joinable()) {
			std::lock_guard<std::mutex> lock(state_mutex_);
			locked = first_ && !finished_;
			return locked ? 0 : -EIO;
		}
		std::vector<std::uint8_t> status;
		if (!reference_.Command(0x13, {}, status, deadline_) || status.size() != 24) return -EIO;
		locked = status[0] == 1;
		if (locked && requested_) {
			const int result = Start();
			if (result) {
				// ReceiverBaseは通常のCheckLockエラーでpollを続ける。暗黙の再Startを防ぐ。
				capture_failed_ = true;
				locked = false;
				return -ECANCELED;
			}
		}
		return 0;
	}
	int SetLnbVoltage(std::int32_t voltage) override { return voltage ? -EINVAL : 0; }
	int ReadStat(px4::command::StatType, std::int32_t &) override { return -ENOSYS; }
	bool WorkerRunning() { std::lock_guard<std::mutex> lock(state_mutex_); return worker_.joinable() && !finished_; }
	int FinishObservation() { return Stop(); }
	std::uint64_t Accepted() const { return accepted_.load(); }
	void SuppressNextTs() { suppress_next_ = true; }
	const TorneCaptureStats &Stats() const { return stats_; } // join後のみ参照
protected:
	int SetStreamId() override { return 0; }
	int SetFrequency() override
	{
		if (!open_ || params_.system != px4::SystemType::ISDB_T || params_.freq < 473143 || params_.freq > 767143) return -EINVAL;
		tuned_ = false;
		capture_failed_ = false;
		if (Stop()) return -EIO;
		if (!stream_buf_->Purge()) return -EIO;
		stream_buf_->ResetInputPacketCount();
		std::vector<std::uint8_t> response, frequency(4);
		const auto hz = params_.freq * 1000u;
		for (unsigned i = 0; i < 4; ++i) frequency[i] = static_cast<std::uint8_t>(hz >> (i * 8));
		if (!reference_.Command(0x11, frequency, response, deadline_)) return -EIO;
		tuned_ = true;
		return 0;
	}
private:
	int Start()
	{
		if (worker_.joinable()) return WorkerRunning() ? 0 : -EIO;
		std::vector<std::uint8_t> response;
		const std::uint8_t output_mode = suppress_next_ ? 0 : 1;
		suppress_next_ = false;
		if (!reference_.Command(0x12, {output_mode}, response, deadline_)) return -EIO;
		if (!ResetEvent(cancel_)) return -EIO;
		{ std::lock_guard<std::mutex> lock(state_mutex_); first_ = finished_ = false; }
		try {
			worker_ = std::thread([this] {
				try {
					CaptureFirstTs(file_, usb_, cancel_, engine_, deadline_, stats_, MAXDWORD - 1,
						[this](const std::uint8_t *data, std::size_t size) {
							// 既存StreamBufferの入力計数は188バイト単位のWriteを前提とする。
							for (std::size_t offset = 0; offset < size; offset += 188) {
								std::size_t packet = 188;
								if (!stream_buf_->Write(data + offset, packet) || packet != 188) return false;
								++accepted_;
							}
							stream_buf_->NotifyWrite();
							{ std::lock_guard<std::mutex> lock(state_mutex_); first_ = true; }
							changed_.notify_all();
							return true;
						});
				} catch (...) { stats_.error = ERROR_NOT_ENOUGH_MEMORY; }
				{ std::lock_guard<std::mutex> lock(state_mutex_); finished_ = true; }
				changed_.notify_all();
			});
		} catch (...) { return -ENOMEM; }
		std::unique_lock<std::mutex> lock(state_mutex_);
		const auto first_deadline = (std::min)(deadline_, Clock::now() + std::chrono::seconds(2));
		const bool ready = changed_.wait_until(lock, first_deadline, [this] { return first_ || finished_; });
		const bool success = ready && first_ && !finished_;
		lock.unlock();
		if (!success) { Stop(); return -ETIMEDOUT; }
		return 0;
	}
	int Stop()
	{
		if (!worker_.joinable()) return 0;
		SetEvent(cancel_);
		worker_.join(); // read回収とengine.Stop完了前に次の選局や破棄を行わない
		std::vector<std::uint8_t> response;
		const bool parked = reference_.Command(0x12, {0}, response, deadline_);
		return parked && stats_.stop == px4::torne::StreamResult::ok &&
			(!stats_.error || stats_.error == ERROR_OPERATION_ABORTED) ? 0 : -EIO;
	}
	HANDLE file_, cancel_ = nullptr;
	WINUSB_INTERFACE_HANDLE usb_;
	px4::torne::StreamEngine engine_;
	TorneReferenceClient &reference_;
	Clock::time_point deadline_;
	bool open_ = false, requested_ = false, tuned_ = false;
	bool suppress_next_ = false;
	bool capture_failed_ = false;
	std::thread worker_;
	std::mutex state_mutex_;
	std::condition_variable changed_;
	bool first_ = false, finished_ = false;
	TorneCaptureStats stats_;
	std::atomic<std::uint64_t> accepted_{0};
};
}

bool TestTorneReceiver(HANDLE file, WINUSB_INTERFACE_HANDLE usb, px4::torne::MemoryAccess &memory,
	px4::torne::Bootstrap &bootstrap, TorneReferenceClient &reference, Clock::time_point deadline,
	const std::vector<unsigned> &channels, unsigned observation_ms, bool inject_no_ts)
{
	Receiver receiver(file, usb, memory, bootstrap, reference, deadline);
	if (receiver.SetCapture(true) != -EINVAL) return false;
	if (receiver.Open() || receiver.SetCapture(true) || receiver.WorkerRunning()) return false;
	auto buffer = receiver.GetStreamBuffer();
	std::atomic<std::uint64_t> delivered{0};
	std::atomic_bool valid{true};
	std::thread consumer([&] {
		buffer->HandleRead(188u * 128u, [&](const void *data, std::size_t size) {
			const auto *bytes = static_cast<const std::uint8_t *>(data);
			if (size % 188) valid = false;
			for (std::size_t i = 0; i + 188 <= size; i += 188) if (bytes[i] != 0x47) valid = false;
			delivered += size / 188;
			return true;
		});
	});
	bool ok = true;
	if (inject_no_ts) {
		px4::command::ParameterSet parameters{};
		parameters.system = px4::SystemType::ISDB_T;
		parameters.freq = 473143u + (channels.front() - 13u) * 6000u;
		receiver.SuppressNextTs();
		const auto begin = Clock::now();
		const bool unexpected = receiver.SetParameters(parameters) && receiver.Tune(5000);
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - begin).count();
		const int stopped = receiver.FinishObservation();
		const auto &failed_stats = receiver.Stats();
		ok = !unexpected && !stopped && !receiver.WorkerRunning() &&
			failed_stats.start == px4::torne::StreamResult::ok && failed_stats.stop == px4::torne::StreamResult::ok &&
			failed_stats.error == ERROR_OPERATION_ABORTED;
		ok = ok && receiver.SetCapture(true) == -ECANCELED && !receiver.WorkerRunning();
		std::cout << "INJECTED NO TS expected_failure=" << ok << " elapsed_ms=" << elapsed
			<< " packets=" << failed_stats.packets << " io_error=" << failed_stats.error
			<< " first_ts_ms=" << failed_stats.first_ts_ms << " max_read_gap_ms=" << failed_stats.max_read_gap_ms << '\n';
	}
	for (std::size_t channel_index = 0; channel_index < channels.size(); ++channel_index) {
		if (!ok) break;
		const auto channel = channels[channel_index];
		px4::command::ParameterSet parameters{};
		parameters.system = px4::SystemType::ISDB_T;
		parameters.freq = 473143u + (channel - 13u) * 6000u;
		const auto before = delivered.load();
		ok = receiver.SetParameters(parameters) && receiver.Tune(5000);
		const auto until = (std::min)(deadline, Clock::now() + std::chrono::milliseconds(observation_ms));
		while (ok && Clock::now() < until) {
			if (!receiver.WorkerRunning()) { ok = false; break; }
			Sleep(10);
		}
		const int stopped = receiver.FinishObservation();
		const auto &stats = receiver.Stats();
		std::cout << "RECEIVER channel=" << channel << " tune=" << ok << " stop=" << stopped
			<< " input_packets=" << buffer->GetInputPacketCount() << " delivered_total=" << delivered.load()
			<< " tei=" << stats.tei << " cc_anomalies=" << stats.cc_anomalies << " malformed=" << stats.malformed << '\n';
		ok = ok && !stopped && valid && delivered.load() > before;
		if (!ok) break;
		if (channel_index + 1 < channels.size() && (receiver.SetCapture(true) || !receiver.WorkerRunning())) { ok = false; break; }
	}
	// 前のepochの再開分も含め、worker停止後に総投入数と総読出し数を照合する。
	receiver.FinishObservation();
	const auto drain_deadline = (std::min)(deadline, Clock::now() + std::chrono::seconds(1));
	while (delivered.load() < receiver.Accepted() && Clock::now() < drain_deadline) Sleep(1);
	ok = ok && delivered.load() == receiver.Accepted();
	std::cout << "RECEIVER BUFFER accepted=" << receiver.Accepted() << " delivered=" << delivered.load() << " equal=" << ok << '\n';
	receiver.Close();
	consumer.join();
	ok = ok && receiver.SetCapture(true) == -EINVAL;
	receiver.Close(); // 冪等なcloseも診断する
	std::cout << "RECEIVER BASE INTEGRATION " << (ok ? "PASSED" : "FAILED") << " (no BonDriver/host registration)\n";
	return ok;
}
