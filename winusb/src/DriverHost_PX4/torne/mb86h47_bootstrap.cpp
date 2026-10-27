// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
// See mb86h47_bootstrap.hpp for origin and adaptation scope.
#include "mb86h47_bootstrap.hpp"
#include "mb86h47_programs.hpp"

#include <algorithm>
#include <array>

namespace px4::torne {
namespace {
constexpr std::uint32_t core1_base = 0x1700000;
constexpr std::uint32_t origin = 0xa0000;
constexpr std::array<std::uint32_t, 2> banks{0xe1000, 0xe1800};

int AddressingBank(std::uint32_t address) noexcept
{
	for (int bank = 0; bank < 2; ++bank)
		if (address >= banks[bank] && address < banks[bank] + 0x800)
			return bank;
	return -1;
}

bool JumpTarget(std::uint32_t word, std::uint32_t &target) noexcept
{
	if (((word >> 16) & 0xff0f) != 0x9b0c)
		return false;
	target = ((word >> 20) & 0xf) << 16 | (word & 0xffff);
	return true;
}
}

void Bootstrap::ClearError() noexcept
{
	result_ = BootstrapResult::ok;
	memory_result_ = MemoryResult::ok;
	system_error_ = 0;
	last_address_ = 0;
}

void Bootstrap::Block(std::uint32_t address, std::uint8_t *data, std::size_t size, bool read)
{
	if (result_ != BootstrapResult::ok)
		return;
	last_address_ = address;
	memory_result_ = memory_.Transfer(address, read, data, size);
	if (memory_result_ != MemoryResult::ok) {
		system_error_ = memory_.LastTransportError();
		result_ = BootstrapResult::memory_error;
	}
}

std::uint32_t Bootstrap::Read(std::uint32_t address)
{
	std::array<std::uint8_t, 4> bytes{};
	Block(address, bytes.data(), bytes.size(), true);
	return (std::uint32_t(bytes[0]) << 24) | (std::uint32_t(bytes[1]) << 16) |
		(std::uint32_t(bytes[2]) << 8) | bytes[3];
}

void Bootstrap::Write(std::uint32_t address, std::uint32_t value)
{
	std::array<std::uint8_t, 4> bytes{ static_cast<std::uint8_t>(value >> 24),
		static_cast<std::uint8_t>(value >> 16), static_cast<std::uint8_t>(value >> 8),
		static_cast<std::uint8_t>(value) };
	Block(address, bytes.data(), bytes.size(), false);
}

void Bootstrap::Write16(std::uint32_t address, std::uint16_t value)
{
	const auto previous = Read(address);
	Write(address, (std::uint32_t(value) << 16) | (previous & 0xffff));
}

void Bootstrap::Pause(unsigned milliseconds)
{
	if (result_ != BootstrapResult::ok)
		return;
	system_error_ = wait_.Pause(milliseconds);
	if (system_error_)
		result_ = BootstrapResult::wait_error;
}

void Bootstrap::Widen()
{
	const auto &fresh = programs::patches0;
	std::array<std::uint32_t, programs::patches0.size()> held{};
	int bank = -1;
	std::size_t jumps = 0, widened = 0;
	for (std::size_t i = 0; i < fresh.size(); ++i) {
		held[i] = Read(fresh[i].address);
		if (result_ != BootstrapResult::ok)
			return;
		std::uint32_t target = 0;
		const bool jump = JumpTarget(held[i], target);
		if (held[i] == fresh[i].original)
			continue;
		if (held[i] == fresh[i].replacement && !jump) {
			++widened;
		} else if (jump && AddressingBank(target) >= 0 && (bank < 0 || bank == AddressingBank(target))) {
			bank = AddressingBank(target);
			++jumps;
		} else {
			result_ = BootstrapResult::unknown_handler;
			return;
		}
	}
	if (jumps + widened < fresh.size()) {
		// 分岐の設置前はbank0が未使用。coaxと同じ順序でワード単位に切り替える。
		bank = 0;
	} else if (bank >= 0) {
		const auto &patches = bank == 0 ? programs::patches0 : programs::patches1;
		const auto &code = bank == 0 ? programs::addressing0 : programs::addressing1;
		bool same = true;
		for (std::size_t i = 0; i < patches.size(); ++i)
			same = same && held[i] == patches[i].replacement;
		if (same) {
			for (std::size_t i = 0; i < code.size() && result_ == BootstrapResult::ok; ++i)
				same = same && Read(banks[bank] + static_cast<std::uint32_t>(i * 4)) == code[i];
			if (result_ != BootstrapResult::ok || same)
				return;
		}
		// 使用中のbankを書き換えると要求処理の途中でコードが混ざるため反対側へロード。
		bank = 1 - bank;
	} else {
		result_ = BootstrapResult::unknown_handler;
		return;
	}
	const auto &code = bank == 0 ? programs::addressing0 : programs::addressing1;
	const auto &patches = bank == 0 ? programs::patches0 : programs::patches1;
	for (std::size_t i = 0; i < code.size(); ++i)
		Write(banks[bank] + static_cast<std::uint32_t>(i * 4), code[i]);
	for (std::size_t i = 0; i < code.size() && result_ == BootstrapResult::ok; ++i) {
		const auto value = Read(banks[bank] + static_cast<std::uint32_t>(i * 4));
		if (result_ == BootstrapResult::ok && value != code[i])
			result_ = BootstrapResult::verification_failed;
	}
	// 検証失敗時はhandler側を一切切り替えない。
	for (const auto &patch : patches)
		Write(patch.address, patch.replacement);
}

void Bootstrap::Clocks()
{
	Write16(0xf1010c, 0x8201);
	Write16(0xf10114, 0x8008);
	Write16(0xf10118, 0xffff);
	for (const std::uint32_t base : {0x14d0000u, 0x10d0000u, 0x12d0000u, 0x13d0000u,
		0xe70000u, 0xc70000u, 0xd00000u, 0x1600000u, core1_base}) {
		const auto previous = Read(base + 0x404);
		Write(base + 0x404, previous | (4u << 16));
	}
	Write16(0xf1011c, 6);
	Write16(0xf10128, 0x6000);
	Write16(0xf1012c, 0x4007);
	Write16(0xf12144, 3);
	for (const std::uint32_t address : {0xf12178u, 0xf1217cu, 0xf12198u, 0xf1219cu})
		Write16(address, 0x880);
}

void Bootstrap::Traps()
{
	for (std::uint32_t offset = 0; offset < 0x40; offset += 4) {
		const auto value = Read(0x440 + offset);
		if ((value | 0x1f1f1f1f) != value)
			Write(0x440 + offset, value | 0x1f1f1f1f);
	}
	for (std::size_t i = 0; i < programs::trapRecorder.size(); ++i)
		Write(0xe3800 + static_cast<std::uint32_t>(i * 4), programs::trapRecorder[i]);
	for (std::size_t i = 0; i < programs::trapStubs.size(); ++i)
		Write(0xe3000 + static_cast<std::uint32_t>(i * 4), programs::trapStubs[i]);
	for (std::uint32_t vector = 1; vector < 256; ++vector) {
		if (vector >= 16 && vector < 20)
			continue;
		Write(0xffc00 + 0x3fc - 4 * vector, 0xe3000 + vector * 8);
	}
}

void Bootstrap::I2cInit()
{
	const auto enabled = Read(0xfd8010);
	Write(0xfd8010, enabled | 0x10);
	const auto reset = Read(0xfd8014);
	Write(0xfd8014, reset & ~0x10u);
	Write(0xfd80c0, 1);
	Write(0xfd80c0, 0);
	Write(0xfd80c4, 0);
	Write16(0x90114, 0x8008);
	Write(0xfd8ff8, 6);
	Write(0xfd8ff4, 8);
	Write(0xfd8fe8, 0x3c);
	Write(0xfd8fe4, 0x40);
}

void Bootstrap::Load(std::uint32_t base, const std::uint32_t *program, std::size_t count)
{
	Write(base + 0xffffc, origin);
	Write(base, 0xffffffff);
	for (std::size_t first = 0; first < count && result_ == BootstrapResult::ok; first += 16) {
		const auto words = (std::min)(std::size_t(16), count - first);
		const auto address = base + origin + static_cast<std::uint32_t>(first * 4);
		std::array<std::uint8_t, 64> loaded{}, wanted{};
		Block(address, loaded.data(), words * 4, true);
		for (std::size_t i = 0; i < words; ++i) {
			const auto word = program[first + i];
			for (unsigned byte = 0; byte < 4; ++byte)
				wanted[i * 4 + byte] = static_cast<std::uint8_t>(word >> (24 - byte * 8));
		}
		if (!std::equal(wanted.begin(), wanted.begin() + words * 4, loaded.begin()))
			Block(address, wanted.data(), words * 4, false);
	}
}

void Bootstrap::Core1(std::uint32_t seed)
{
	Load(core1_base, programs::core1.data(), programs::core1.size());
	for (const std::uint32_t offset : {0xa3f00u, 0xa3f80u, 0xa3f7cu, 0xa3d00u, 0xa3d40u, 0xa3e44u})
		Write(core1_base + offset, 0);
	std::array<std::uint8_t, 64> zero{};
	Block(core1_base + 0xa3e00, zero.data(), zero.size(), false);
	Write(core1_base + 0xa3c04, 0x82100000);
	Write(core1_base + 0xa3c14, 0);
	Write(core1_base + 0xa3c00, seed);
	Write(core1_base + 0xa3e40, 15u << 24);
	Write(core1_base, 0xfffcffff);
	const auto before = Read(core1_base + 0xa3f7c);
	Pause(1);
	const auto after = Read(core1_base + 0xa3f7c);
	if (result_ == BootstrapResult::ok && before == after)
		result_ = BootstrapResult::core1_not_running;
}

BootstrapResult Bootstrap::Initialize(std::uint32_t command_seed)
{
	ClearError();
	initialized_ = false;
	if (!command_seed)
		return result_ = BootstrapResult::invalid_seed;
	const auto state = Read(0x82008);
	if (result_ == BootstrapResult::ok && (state >> 16) != 0)
		return result_ = BootstrapResult::not_power_up_state;
	Widen();
	Clocks();
	Traps();
	I2cInit();
	Core1(command_seed);
	initialized_ = result_ == BootstrapResult::ok;
	if (initialized_) {
		++generation_;
		if (!generation_)
			++generation_;
	}
	return result_;
}

BootstrapResult Bootstrap::ResetDemodulator()
{
	ClearError();
	if (!initialized_)
		return result_ = BootstrapResult::not_initialized;
	// 呼出し前にI2C要求を回収済みであること。ring/mailboxを動かしたままresetしない。
	const auto value = Read(0x90018) >> 16;
	const auto direction = Read(0x90014) >> 16;
	Write16(0x90018, static_cast<std::uint16_t>((value & ~1u) | (1u << 6)));
	Write16(0x90014, static_cast<std::uint16_t>(direction | 1u | (1u << 6)));
	Pause(50);
	Write16(0x90018, static_cast<std::uint16_t>(value | 1u | (1u << 6)));
	Pause(10);
	if (result_ != BootstrapResult::ok)
		initialized_ = false;
	return result_;
}

} // namespace px4::torne
