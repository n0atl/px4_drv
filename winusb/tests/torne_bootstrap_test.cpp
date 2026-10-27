// SPDX-License-Identifier: GPL-2.0-only
#include "../src/DriverHost_PX4/torne/mb86h47_bootstrap.hpp"
#include "../src/DriverHost_PX4/torne/mb86h47_programs.hpp"
#include "../src/DriverHost_PX4/torne/mb86h47_i2c.hpp"
#include "../src/DriverHost_PX4/torne/dib8096_registers.hpp"
#include "../src/DriverHost_PX4/torne/mb86h47_stream.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

using namespace px4::torne;

static void Check(bool condition, const char *message)
{
	if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Chip final : public ControlTransport, public BootstrapWait {
public:
	Chip()
	{
		for (const auto &patch : programs::patches0)
			Put(patch.address, patch.original);
		for (unsigned vector = 0; vector < 256; ++vector)
			Put(0xffc00 + 0x3fc - 4 * vector, 0xdead0000 + vector);
		Put(0xf1010c, 0x12345678);
		Put(0x90018, 0x00021234);
		Put(0x90014, 0x00205678);
	}
	void Put(std::uint32_t address, std::uint32_t word)
	{
		for (unsigned i = 0; i < 4; ++i)
			memory[address + i] = static_cast<std::uint8_t>(word >> (24 - i * 8));
	}
	std::uint32_t Get(std::uint32_t address)
	{
		std::uint32_t word = 0;
		for (unsigned i = 0; i < 4; ++i)
			word = (word << 8) | memory[address + i];
		return word;
	}
	int Transfer(const MemoryRequest &request, std::uint8_t *data, std::size_t length, std::size_t &transferred) override
	{
		++calls;
		now += std::chrono::microseconds(100);
		const auto address = (std::uint32_t(request.value & 0xff00) << 8) |
			(std::uint32_t(request.value & 0xff) << 24) | request.index;
		if (calls == fail_call) { transferred = 0; return 1167; }
		if (calls == short_call) { transferred = length - 1; return 0; }
		if (request.type == 0xc0) {
			for (std::size_t i = 0; i < length; ++i)
				data[i] = memory[address + static_cast<std::uint32_t>(i)];
			if (corrupt_address && address == corrupt_address)
				data[0] ^= 1;
		} else {
			writes.push_back(address);
			for (std::size_t i = 0; i < length; ++i)
				memory[address + static_cast<std::uint32_t>(i)] = data[i];
			if (address == 0xfdbd08 && !hold_endpoint_reset)
				Put(address, Get(address) & ~1u);
			if (address == 0x1700000)
				running = Get(address) == 0xfffcffff;
			if (address == 0x17a3f00 && answer_mailbox) {
				const auto command = Get(address);
				mailbox_sequences.push_back(static_cast<std::uint8_t>(command >> 24));
				Put(0x17a3f80, (command & 0xff000000) | mailbox_error);
				for (unsigned i = 0; i < (command & 0xff); ++i)
					memory[0x17a3f84 + i] = static_cast<std::uint8_t>(i + 0x20);
				if (identify_chip && (command & 0xffffff) == 0x120202) {
					const auto number = Get(0x17a3f04) >> 16;
					const std::uint16_t value = number == 896 ? (++vendor_reads == 1 ? 0xdead : vendor_id) : device_id;
					Put(0x17a3f84, std::uint32_t(value) << 16);
				}
			}
		}
		transferred = length;
		return 0;
	}
	int Pause(unsigned milliseconds) override
	{
		pauses.push_back(milliseconds);
		if (pause_error)
			return pause_error;
		if (running && !stuck)
			Put(0x17a3f7c, Get(0x17a3f7c) + 1);
		return 0;
	}
	std::chrono::steady_clock::time_point Now() const noexcept override { return now; }
	std::map<std::uint32_t, std::uint8_t> memory;
	std::vector<std::uint32_t> writes;
	std::vector<unsigned> pauses;
	std::size_t calls = 0, fail_call = 0, short_call = 0;
	std::uint32_t corrupt_address = 0;
	int pause_error = 0;
	bool running = false, stuck = false;
	bool answer_mailbox = false;
	std::uint8_t mailbox_error = 0;
	std::vector<std::uint8_t> mailbox_sequences;
	std::chrono::steady_clock::time_point now{};
	bool identify_chip = false;
	unsigned vendor_reads = 0;
	std::uint16_t vendor_id = 0x01b3, device_id = 0x8090;
	bool hold_endpoint_reset = false;
};

static void StreamTests()
{
	Chip chip;
	MemoryAccess memory(chip);
	Bootstrap boot(memory,chip);
	Check(boot.Initialize(1)==BootstrapResult::ok,"stream test bootstrap");
	StreamEngine engine(memory,boot);
	const auto before=chip.calls;
	Check(engine.Start()==StreamResult::invalid_state && chip.calls==before,"stream cannot start without preparation");
	Check(engine.Prepare()==StreamResult::ok,"stream preparation");
	const auto preparation_calls=chip.calls-before;
	Check(chip.Get(0xd01040)==0 && chip.Get(0xd02010)==0,"prepare holds input and ring writer");
	Check(chip.Get(0xd02020)==0xae4000 && chip.Get(0xd02024)==0x600000,"stream SDRAM ring layout");
	Check(chip.Get(0xfdbc14)==0x00060030 && chip.Get(0xfd8204)==0xb09f0000,"EP1 DMA mode and buffers");
	Check(engine.Start()==StreamResult::ok && chip.Get(0xd01040)==0x03000000 && chip.Get(0xd02010)==0x02000000,"stream input/writer enable");
	Check(engine.Prepare()==StreamResult::invalid_state,"cannot reprepare running stream");
	Check(engine.Stop()==StreamResult::ok && chip.Get(0x1600000)==0xffffffff && chip.Get(0xd01040)==0 && chip.Get(0xd02010)==0x04000000,"stream stop order endpoints");
	Check(engine.Prepare()==StreamResult::ok,"stream reprepare after stop");
	Check(boot.Initialize(2)==BootstrapResult::ok,"restart core generation");
	const auto old_generation=chip.calls;
	Check(engine.Start()==StreamResult::invalid_state && engine.Stop()==StreamResult::invalid_state && chip.calls==old_generation,"old stream generation rejects hardware operations");
	Chip stuck;
	MemoryAccess stuck_memory(stuck);
	Bootstrap stuck_boot(stuck_memory,stuck);
	Check(stuck_boot.Initialize(1)==BootstrapResult::ok,"endpoint test bootstrap");
	stuck.hold_endpoint_reset=true;
	StreamEngine stuck_engine(stuck_memory,stuck_boot);
	Check(stuck_engine.Prepare()==StreamResult::endpoint_reset_failed,"EP reset remaining busy is a failure");
	for(std::size_t failure=1;failure<=preparation_calls;++failure) {
		Chip bad;
		MemoryAccess mem(bad);
		Bootstrap bootstrap(mem,bad);
		Check(bootstrap.Initialize(1)==BootstrapResult::ok,"failure fixture bootstrap");
		const auto begin=bad.calls;
		bad.fail_call=begin+failure;
		StreamEngine stream(mem,bootstrap);
		Check(stream.Prepare()==StreamResult::memory_error && bad.calls==begin+failure,"stream prepare stops after first failed USB request");
	}
}

static void I2cTests()
{
	Chip chip;
	MemoryAccess memory(chip);
	Bootstrap boot(memory, chip);
	MailboxI2c i2c(memory, chip);
	Check(!i2c.BeginAfterBootstrap(boot), "mailbox requires initialized core1");
	Check(boot.Initialize(1) == BootstrapResult::ok && i2c.BeginAfterBootstrap(boot), "mailbox starts after bootstrap");
	chip.answer_mailbox = true;
	std::array<std::uint8_t, 3> write{0x05, 0x07, 0x12};
	std::array<std::uint8_t, 32> read{};
	auto transfer = [&] {
		return i2c.Transfer(0x12, write.data(), write.size(), read.data(), read.size(), chip.Now() + std::chrono::seconds(1));
	};
	Check(transfer() == I2cResult::ok, "mailbox repeated-start transfer");
	Check(chip.Get(0x17a3f04) == 0x05071200, "mailbox payload big-endian and padded");
	Check(chip.Get(0x17a3f00) == 0x01120320, "mailbox command includes exact sizes and 8-bit slave address");
	for (unsigned i = 0; i < read.size(); ++i)
		Check(read[i] == i + 0x20, "mailbox copies successful response");
	for (unsigned i = 0; i < 260; ++i)
		Check(transfer() == I2cResult::ok, "mailbox sequential transfers across wrap");
	for (auto sequence : chip.mailbox_sequences)
		Check(sequence != 0, "mailbox skips zero sequence");
	chip.mailbox_error = 2;
	read.fill(0xee);
	Check(transfer() == I2cResult::address_nack && read[0] == 0xee, "NACK cannot publish read data");
	chip.mailbox_error = 0;
	Check(transfer() == I2cResult::ok, "completed NACK leaves bus reusable");
	const auto calls = chip.calls;
	Check(i2c.Transfer(0x13, write.data(), write.size(), nullptr, 0, chip.Now() + std::chrono::seconds(1)) == I2cResult::invalid_argument && chip.calls == calls, "reject odd 8-bit address without USB");
	chip.answer_mailbox = false;
	Check(i2c.Transfer(0x12, write.data(), write.size(), read.data(), 2, chip.Now() + std::chrono::milliseconds(2)) == I2cResult::timeout, "stale mailbox sequence hits absolute deadline");
	const auto after_timeout = chip.calls;
	Check(transfer() == I2cResult::not_ready && chip.calls == after_timeout, "late answer cannot be reused after timeout");
	Check(!i2c.BeginAfterBootstrap(boot), "same bootstrap generation cannot clear timeout poison");
	Check(boot.Initialize(2) == BootstrapResult::ok && i2c.BeginAfterBootstrap(boot), "new bootstrap reestablishes mailbox session");
	chip.answer_mailbox = true;
	chip.mailbox_error = 4;
	Check(transfer() == I2cResult::stuck && transfer() == I2cResult::not_ready, "stuck bus invalidates session");
	Check(boot.Initialize(3) == BootstrapResult::ok && i2c.BeginAfterBootstrap(boot), "new core generation after stuck bus");
	chip.mailbox_error = 0;
	Check(boot.Initialize(4) == BootstrapResult::ok, "external bootstrap changes generation");
	const auto before_old_generation = chip.calls;
	Check(transfer() == I2cResult::not_ready && chip.calls == before_old_generation, "old mailbox generation cannot address a restarted core");
	Check(i2c.BeginAfterBootstrap(boot) && transfer() == I2cResult::ok, "explicit attachment to new generation");
	chip.identify_chip = true;
	DiB8096Registers registers(i2c);
	Check(registers.Write(1287, 3, chip.Now() + std::chrono::seconds(1)) == I2cResult::ok && chip.Get(0x17a3f04) == 0x05070003, "demod enumeration register encoding");
	std::uint16_t vendor = 0, device = 0;
	bool recognized = false;
	Check(registers.Identify(vendor, device, recognized, chip.Now() + std::chrono::seconds(1)) == I2cResult::ok && recognized && vendor == 0x01b3 && device == 0x8090 && chip.vendor_reads == 2, "demod identification discards first vendor glitch");
	chip.vendor_id = 0xdead;
	Check(registers.Identify(vendor, device, recognized, chip.Now() + std::chrono::seconds(1)) == I2cResult::ok && !recognized && device == 0, "wrong vendor cannot reuse old identity");
}

int main()
{
	I2cTests();
	StreamTests();
	Chip chip;
	MemoryAccess memory(chip);
	Bootstrap boot(memory, chip);
	Check(boot.ResetDemodulator() == BootstrapResult::not_initialized && chip.calls == 0, "reset requires successful bootstrap");
	Check(boot.Initialize(0) == BootstrapResult::invalid_seed && chip.calls == 0, "zero seed rejected before USB");
	Check(boot.Initialize(0x1234567) == BootstrapResult::ok, "cold bootstrap");
	const auto total_calls = chip.calls;
	Check(chip.Get(0xf1010c) == 0x82015678, "clock RMW retains lower half");
	Check(chip.Get(0x17a3c00) == 0x1234567 && chip.Get(0x17a3c04) == 0x82100000, "core command sequence and slot offsets");
	Check(chip.Get(0x17a3e40) == 0x0f000000 && chip.Get(0x1700000) == 0xfffcffff, "ring initial lap and core release");
	for (unsigned vector : {0u, 16u, 17u, 18u, 19u})
		Check(chip.Get(0xffc00 + 0x3fc - 4 * vector) == 0xdead0000 + vector, "USB and reset vectors preserved");
	Check(chip.Get(0xffc00 + 0x3fc - 4 * 20) == 0xe3000 + 20 * 8, "remaining vectors use trap recorder");
	Check(boot.ResetDemodulator() == BootstrapResult::ok, "demod power/reset");
	Check(chip.Get(0x90018) == 0x00431234 && chip.Get(0x90014) == 0x00615678, "GPIO bit and low-half preservation");
	Check(chip.pauses == std::vector<unsigned>({1, 50, 10}), "original reset settling waits retained");
	chip.writes.clear();
	Check(boot.Initialize(123) == BootstrapResult::ok, "same-version warm bootstrap");
	for (const auto address : chip.writes)
		Check(!(address >= 0xe1000 && address < 0xe2000), "existing addressing code reused");
	chip.Put(0xe1000, chip.Get(0xe1000) ^ 1);
	chip.writes.clear();
	Check(boot.Initialize(456) == BootstrapResult::ok, "stale active bank migrates");
	for (const auto address : chip.writes)
		Check(!(address >= 0xe1000 && address < 0xe1800), "active bank never overwritten");
	for (const auto &patch : programs::patches1)
		Check(chip.Get(patch.address) == patch.replacement, "handler switches to verified opposite bank");

	for (int scenario = 0; scenario < 5; ++scenario) {
		Chip bad;
		MemoryAccess mem(bad);
		Bootstrap init(mem, bad);
		if (scenario == 0) bad.Put(0x82008, 0x00010000);
		if (scenario == 1) bad.Put(programs::patches0[0].address, 0xdeadbeef);
		if (scenario == 2) bad.corrupt_address = 0xe1000;
		if (scenario == 3) bad.stuck = true;
		if (scenario == 4) bad.pause_error = 995;
		const auto expected = std::vector<BootstrapResult>{BootstrapResult::not_power_up_state,
			BootstrapResult::unknown_handler, BootstrapResult::verification_failed,
			BootstrapResult::core1_not_running, BootstrapResult::wait_error};
		Check(init.Initialize(1) == expected[scenario], "bootstrap error classification");
		if (scenario < 2) Check(bad.writes.empty(), "unsafe state rejected without writes");
		if (scenario == 2)
			for (const auto &patch : programs::patches0)
				Check(bad.Get(patch.address) == patch.original, "SRAM verify failure leaves handler untouched");
	}
	// 各ステップへ失敗を注入し、最初の失敗後に追加のUSB要求が出ないことを確認する。
	for (std::size_t fail = 1; fail <= total_calls; ++fail) {
		Chip bad;
		bad.fail_call = fail;
		MemoryAccess mem(bad);
		Bootstrap init(mem, bad);
		Check(init.Initialize(1) == BootstrapResult::memory_error && bad.calls == fail && init.LastSystemError() == 1167, "every transfer failure stops subsequent I/O");
	}
	Chip short_io;
	short_io.short_call = 2;
	MemoryAccess short_memory(short_io);
	Bootstrap short_boot(short_memory, short_io);
	Check(short_boot.Initialize(1) == BootstrapResult::memory_error && short_boot.LastMemoryResult() == MemoryResult::short_transfer && short_io.writes.empty(), "short handler read cannot trigger writes");
	std::cout << "PASS: torne bootstrap (" << total_calls << " injected transfer failures; no hardware access)\n";
}
