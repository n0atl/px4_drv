// SPDX-License-Identifier: GPL-2.0-only
#include "../src/DriverHost_PX4/torne/mb86h47_protocol.hpp"
#include "../src/DriverHost_PX4/torne/stream_framer.hpp"
#include "../src/DriverHost_PX4/torne/winusb_control.hpp"
#include "../src/DriverHost_PX4/torne/device_identity.hpp"
#include "../src/DriverHost_PX4/torne/ts_quality.hpp"
#include "../src/DriverHost_PX4/torne/mb86h47_card.hpp"
#include "../src/DriverHost_PX4/torne/tune_result.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace px4::torne;

class MockControlIo final : public ControlIo {
public:
	DWORD Submit(WINUSB_INTERFACE_HANDLE, const MemoryRequest &, std::uint8_t *data,
		USHORT length, OVERLAPPED &operation) noexcept override
	{
		++submits;
		buffer = data;
		actual = length;
		if (expire)
			owner->Begin(WinUsbControl::Clock::now() - std::chrono::seconds(1));
		if (cancel_on_submit)
			SetEvent(cancel_event);
		if (signal_completion)
			SetEvent(operation.hEvent);
		return submit_result;
	}
	DWORD Collect(WINUSB_INTERFACE_HANDLE, OVERLAPPED &operation, ULONG &transferred, bool wait) noexcept override
	{
		++collects;
		lifetime_ok = wait && buffer && GetHandleInformation(operation.hEvent, &event_flags);
		transferred = actual;
		return collect_result;
	}
	DWORD Cancel(HANDLE, OVERLAPPED &operation) noexcept override
	{
		++cancels;
		SetEvent(operation.hEvent);
		return cancel_result;
	}
	WinUsbControl *owner = nullptr;
	HANDLE cancel_event = nullptr;
	std::uint8_t *buffer = nullptr;
	ULONG actual = 0;
	DWORD event_flags = 0;
	DWORD submit_result = ERROR_IO_PENDING;
	DWORD collect_result = ERROR_SUCCESS;
	DWORD cancel_result = ERROR_SUCCESS;
	bool signal_completion = true;
	bool expire = false;
	bool cancel_on_submit = false;
	bool lifetime_ok = false;
	int submits = 0, collects = 0, cancels = 0;
};

static void Require(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}

class MockControl final : public ControlTransport {
public:
	int Transfer(const MemoryRequest &request, std::uint8_t *data,
		std::size_t length, std::size_t &transferred) override
	{
		++calls;
		last = request;
		if (fail_on_call && calls == fail_on_call) { transferred = 0; return 1167; }
		if (request.type == 0xc0)
			std::copy(read_data.begin(), read_data.begin() + length, data);
		else
			written.assign(data, data + length);
		transferred = short_io ? length - 1 : length;
		return error;
	}
	MemoryRequest last{};
	std::array<std::uint8_t, 64> read_data{ 0x12, 0x34, 0x56, 0x78 };
	std::vector<std::uint8_t> written;
	int calls = 0;
	int fail_on_call = 0;
	int error = 0;
	bool short_io = false;
};

static void MemoryTests()
{
	const auto request = MakeMemoryRequest(0x82123456, true);
	Require(request.type == 0xc0 && request.request == 0xbc && request.value == 0x1282 && request.index == 0x3456, "32-bit address lanes");
	Require(MakeMemoryRequest(0x00d02038, false).value == 0xd000, "low address lanes");
	MockControl io;
	MemoryAccess memory(io);
	std::uint32_t value = 0;
	Require(memory.Read32(0x1234, value) == MemoryResult::ok && value == 0x12345678, "big endian read");
	Require(memory.Write32(0x1234, 0x89abcdef) == MemoryResult::ok && io.written == std::vector<std::uint8_t>({0x89, 0xab, 0xcd, 0xef}), "big endian write");
	Require(memory.Write16(0x1234, 0xabcd) == MemoryResult::ok && io.written == std::vector<std::uint8_t>({0xab, 0xcd, 0x56, 0x78}), "16-bit RMW preserves low half");
	io.short_io = true;
	value = 0xfeedface;
	Require(memory.Read32(0, value) == MemoryResult::short_transfer && value == 0xfeedface, "short read cannot publish a register value");
	Require(memory.Write32(0, 0) == MemoryResult::short_transfer, "short write is failure");
	const int calls = io.calls;
	Require(memory.Write16(0, 1) == MemoryResult::short_transfer && io.calls == calls + 1, "failed RMW read must not write");
	io.error = 1167;
	Require(memory.Read32(0, value) == MemoryResult::transport_error && memory.LastTransportError() == 1167, "preserve transport error");
	std::array<std::uint8_t, 65> block{};
	const int before = io.calls;
	Require(memory.Transfer(0, true, block.data(), 65) == MemoryResult::invalid_argument, "reject oversized memory request");
	Require(memory.Transfer(0xffffffff, true, block.data(), 2) == MemoryResult::invalid_argument, "reject address overflow");
	Require(memory.Transfer(0, true, nullptr, 4) == MemoryResult::invalid_argument && io.calls == before, "invalid request never reaches USB");
}

static void ControlTests()
{
	HANDLE cancel = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	Require(cancel != nullptr, "create cancel event");
	for (int scenario = 0; scenario < 8; ++scenario) {
		ResetEvent(cancel);
		MockControlIo io;
		WinUsbControl control(reinterpret_cast<HANDLE>(2), reinterpret_cast<WINUSB_INTERFACE_HANDLE>(1), cancel, io);
		io.owner = &control;
		io.cancel_event = cancel;
		control.Begin(WinUsbControl::Clock::now() + std::chrono::seconds(10));
		std::array<std::uint8_t, 4> data{};
		std::size_t transferred = 99;
		if (scenario == 1) io.submit_result = ERROR_SUCCESS;
		if (scenario == 2) { io.signal_completion = false; io.expire = true; }
		if (scenario == 3) io.cancel_on_submit = true;
		if (scenario == 4) SetEvent(cancel);
		if (scenario == 5) io.submit_result = ERROR_DEVICE_NOT_CONNECTED;
		if (scenario == 6) io.collect_result = ERROR_GEN_FAILURE;
		if (scenario == 7) { io.cancel_on_submit = true; io.cancel_result = ERROR_NOT_FOUND; }
		const int result = control.Transfer(MakeMemoryRequest(0x82008, true), data.data(), data.size(), transferred);
		if (scenario < 2)
			Require(result == 0 && transferred == 4 && io.collects == 1 && io.cancels == 0 && io.lifetime_ok, "sync/pending completion collected");
		if (scenario == 2)
			Require(result == ERROR_TIMEOUT && transferred == 0 && io.cancels == 1 && io.collects == 1 && io.lifetime_ok, "timeout cancels and drains exact operation");
		if (scenario == 3 || scenario == 7)
			Require(result == ERROR_OPERATION_ABORTED && transferred == 0 && io.cancels == 1 && io.collects == 1 && io.lifetime_ok, "cancel/completion race still drains");
		if (scenario == 4)
			Require(result == ERROR_OPERATION_ABORTED && io.submits == 0 && io.collects == 0, "pre-cancelled operation never submitted");
		if (scenario == 5)
			Require(result == ERROR_DEVICE_NOT_CONNECTED && io.collects == 0, "submit error preserved without spurious drain");
		if (scenario == 6)
			Require(result == ERROR_GEN_FAILURE && transferred == 0 && io.collects == 1, "completion error preserved");
	}
	ResetEvent(cancel);
	MockControlIo io;
	WinUsbControl expired(reinterpret_cast<HANDLE>(2), reinterpret_cast<WINUSB_INTERFACE_HANDLE>(1), cancel, io);
	std::array<std::uint8_t, 4> bytes{};
	std::size_t size = 0;
	Require(expired.Transfer(MakeMemoryRequest(0, true), bytes.data(), 4, size) == ERROR_TIMEOUT && io.submits == 0, "deadline required before first transfer");
	Require(expired.Pause(0) == ERROR_TIMEOUT, "settling wait shares operation deadline");
	expired.Begin(WinUsbControl::Clock::now() + std::chrono::seconds(1));
	Require(expired.Pause(0) == ERROR_SUCCESS, "zero settling wait completes without device access");
	SetEvent(cancel);
	Require(expired.Pause(50) == ERROR_OPERATION_ABORTED, "stop interrupts settling wait");
	CloseHandle(cancel);
}

static std::vector<std::uint8_t> Units(std::size_t count, std::vector<std::uint8_t> &expected)
{
	std::vector<std::uint8_t> raw;
	for (std::size_t i = 0; i < count; ++i) {
		// timestampにもpayloadにも偽の同期値を置き、単発の0x47で獲得しないことを確認。
		raw.insert(raw.end(), {0x47, 0x91, 0x47, 0x23});
		std::vector<std::uint8_t> packet(188, static_cast<std::uint8_t>(i + 1));
		packet[0] = 0x47;
		packet[1] = 0;
		packet[2] = 0x31;
		packet[3] = static_cast<std::uint8_t>(0x10 | (i & 15));
		packet[50] = 0x47;
		raw.insert(raw.end(), packet.begin(), packet.end());
		expected.insert(expected.end(), packet.begin(), packet.end());
	}
	return raw;
}

static void FramerTests()
{
	std::vector<std::uint8_t> expected;
	const auto raw = Units(8, expected);
	for (std::size_t split = 0; split <= raw.size(); ++split) {
		StreamFramer framer;
		std::vector<std::uint8_t> output;
		framer.Feed(raw.data(), split, output);
		framer.Feed(raw.data() + split, raw.size() - split, output);
		Require(output == expected && framer.PendingSize() == 0, "every two-read boundary");
	}
	for (std::size_t prefix = 0; prefix < 192; ++prefix) {
		StreamFramer framer;
		std::vector<std::uint8_t> output;
		std::vector<std::uint8_t> input(prefix, 0x47);
		input.insert(input.end(), raw.begin(), raw.end());
		for (auto byte : input)
			framer.Feed(&byte, 1, output);
		Require(output == expected && framer.DiscardedBytes() == prefix, "all initial phases and one-byte reads");
	}
	StreamFramer framer;
	std::vector<std::uint8_t> output;
	framer.Feed(raw.data(), 191, output);
	framer.Reset();
	framer.Feed(raw.data(), raw.size(), output);
	Require(output == expected && framer.Packets() == 8, "reset drops previous channel tail");
	// ロック後のTEIを残し、同期喪失後の雑音を破棄して次の完全な列へ復帰する。
	auto tei = raw;
	tei[5] = 0x80;
	framer.Feed(tei.data(), 192, output);
	Require(framer.TeiPackets() == 1 && output[8 * 188 + 1] == 0x80, "TEI retained after sync");
	const std::vector<std::uint8_t> noise(197, 0x11);
	framer.Feed(noise.data(), noise.size(), output);
	framer.Feed(raw.data(), raw.size(), output);
	Require(framer.DiscardedBytes() == noise.size() && framer.Packets() == 17, "recover after byte loss/noise");
	Require(std::equal(expected.begin(), expected.end(), output.end() - expected.size()), "recovered output bytes");
	framer.Reset();
	output.clear();
	framer.Feed(nullptr, 0, output);
	std::vector<std::uint8_t> garbage(100000, 0x47);
	framer.Feed(garbage.data(), garbage.size(), output);
	Require(output.empty() && framer.PendingSize() < 576, "bounded unsynchronised tail");
	// TEI・予約AFCを境界獲得の根拠から除外する。
	for (auto kind : {0, 1}) {
		framer.Reset();
		auto invalid = raw;
		for (std::size_t i = 0; i < 8; ++i)
			invalid[i * 192 + (kind == 0 ? 5 : 7)] = kind == 0 ? 0x80 : 0;
		framer.Feed(invalid.data(), invalid.size(), output);
		Require(output.empty(), "invalid headers cannot acquire sync");
	}
	// 診断モードはTEIを隠さず、通常モードを変えずに8周期で同期する。
	auto error_units=raw;
	auto error_expected=expected;
	for(std::size_t i=0;i<8;++i){error_units[i*192+5]=0x80;error_expected[i*188+1]=0x80;}
	for(std::size_t split=0;split<=error_units.size();++split){
		StreamFramer errors(true);std::vector<std::uint8_t> actual;
		errors.Feed(error_units.data(),split,actual);
		errors.Feed(error_units.data()+split,error_units.size()-split,actual);
		Require(actual==error_expected && errors.TeiPackets()==8,"diagnostic TEI framing across every boundary");
	}
	StreamFramer errors(true);std::vector<std::uint8_t> actual;
	errors.Feed(garbage.data(),garbage.size(),actual);
	Require(actual.empty() && errors.PendingSize()<8*192,"diagnostic mode still rejects invalid AFC/noise");
}

static void IdentityTests()
{
	const std::wstring a = L"\\\\?\\USB#VID_054C&PID_04B2#PORT_A#{GUID}";
	const std::wstring same = L"\\\\?\\usb#vid_054c&pid_04b2#port_a#{guid}";
	const std::wstring b = L"\\\\?\\usb#vid_054c&pid_04b2#port_b#{guid}";
	Require(InterfaceId(a) == InterfaceId(same), "path identity ignores ASCII case");
	Require(InterfaceId(a) != InterfaceId(b), "different ports have different display IDs");
	std::size_t selected = 0;
	Require(SelectInterface({}, {}, selected) == SelectionResult::not_found, "no implicit device when absent");
	Require(SelectInterface({a}, {}, selected) == SelectionResult::ok && selected == 0, "single implicit device");
	Require(SelectInterface({a, b}, {}, selected) == SelectionResult::ambiguous && selected == 2, "multiple devices need selector");
	const auto id = InterfaceId(a);
	Require(SelectInterface({b, a}, id, selected) == SelectionResult::ok && selected == 1, "selection is independent of enumeration order");
	Require(SelectInterface({a, same}, id, selected) == SelectionResult::ambiguous, "duplicate/colliding IDs fail closed");
	Require(SelectInterface({a}, InterfaceId(b), selected) == SelectionResult::not_found, "unknown ID never falls back to other device");
	Require(SelectInterface({a}, "123", selected) == SelectionResult::invalid_id, "reject short ID");
	Require(SelectInterface({a}, "gggggggggggggggg", selected) == SelectionResult::invalid_id, "reject non-hex ID");
	std::string normalized;
	Require(NormalizeInterfaceId("ABCDEF0123456789", normalized) && normalized == "abcdef0123456789", "uppercase selector");
}

static void QualityTests()
{
	TsQuality quality;
	std::array<std::uint8_t,188> packet{};
	packet[0]=0x47; packet[2]=100; packet[3]=0x1f;
	quality.Feed(packet.data());
	packet[3]=0x10; quality.Feed(packet.data());
	Require(quality.cc_anomalies==0, "CC wraps from 15 to 0");
	quality.Feed(packet.data());
	Require(quality.cc_anomalies==1, "duplicate CC is counted");
	packet[3]=0x35; packet[4]=1; packet[5]=0x80; quality.Feed(packet.data());
	Require(quality.cc_anomalies==1, "discontinuity resets CC");
	packet[1]=0x80; quality.Feed(packet.data());
	Require(quality.tei==1, "TEI is retained and resets CC");
	packet[1]=0; packet[3]=0x10; quality.Feed(packet.data());
	Require(quality.cc_anomalies==1, "post-TEI counter has no predecessor");
	packet[3]=0x30; packet[4]=183; quality.Feed(packet.data());
	Require(quality.malformed==1, "adaptation plus payload requires payload space");
	packet[3]=0xd1; quality.Feed(packet.data());
	Require(quality.scrambled==1, "scrambling header bit is observed without changing payload");
}

class CardWait final : public BootstrapWait {
public:
	int Pause(unsigned) override { ++calls; return error; }
	int calls = 0, error = 0;
};

static void CardTests()
{
	{
		MockControl baseline; MemoryAccess baseline_memory(baseline); CardWait baseline_wait;
		CardUart baseline_card(baseline_memory, baseline_wait);
		Require(baseline_card.Reset(), "mock card reset baseline");
		for (int fault = 1; fault <= baseline.calls; ++fault) {
			MockControl broken; broken.fail_on_call = fault;
			MemoryAccess broken_memory(broken); CardWait broken_wait; CardUart broken_card(broken_memory, broken_wait);
			Require(!broken_card.Reset() && broken.calls == fault, "every reset transport fault prevents subsequent register writes");
		}
	}
	MockControl io; MemoryAccess memory(io); CardWait wait; CardUart card(memory, wait);
	bool present = false;
	io.read_data = {};
	Require(card.Present(present) && present, "card IRQ bit clear means present");
	io.read_data[2] = 4;
	Require(card.Present(present) && !present, "card IRQ bit10 means absent");
	io.error = 1167; present = true;
	Require(!card.Present(present) && present, "failed status cannot publish absent");
	const int before = io.calls;
	Require(!card.Reset() && io.calls == before + 1 && wait.calls == 0, "reset stops at first transport failure");
	io.error = 0; io.read_data = {}; wait.error = 995;
	Require(!card.Reset() && card.WaitError() == 995, "reset delay respects cancellation");
	const int start = io.calls;
	Require(!card.SetBaudrate(115200) && io.calls == start, "unsupported speed never touches device");
	Require(!card.WriteBytes(nullptr, 0) && io.calls == start, "empty frame rejected");
	std::vector<std::uint8_t> bytes(259, 0x55);
	Require(card.WriteBytes(bytes.data(), bytes.size()) && io.written == std::vector<std::uint8_t>({0,0,0,1}), "maximum T1 frame sends only after FIFO population");
	io.short_io = true;
	std::size_t count = 99;
	Require(!card.ReadBytes(bytes.data(), bytes.size(), count) && count == 0, "short FIFO status cannot expose bytes");
}

int main()
{
	const std::uint8_t failed[]{1}, unknown[]{2}, extended[]{1,0};
	Require(IsConfirmedSearchFailure(failed, 1), "only explicit completed search failure stops tuning early");
	Require(!IsConfirmedSearchFailure(nullptr, 0) && !IsConfirmedSearchFailure(unknown, 1) && !IsConfirmedSearchFailure(extended, 2), "legacy and unknown replies preserve lock waiting");
	MemoryTests();
	ControlTests();
	FramerTests();
	IdentityTests();
	QualityTests();
	CardTests();
	std::cout << "PASS: torne foundation (mock USB, framing; no hardware access)\n";
	return 0;
}
