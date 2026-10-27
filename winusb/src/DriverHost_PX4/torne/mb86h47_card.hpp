// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
// C++ adaptation of coax card.go at 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405.
// GPLv2 branch selected; memory-request transport only, no command-channel fallback.
#pragma once
#include <algorithm>
#include "mb86h47_bootstrap.hpp"

namespace px4::torne {

// 所有者がbootstrap完了・制御mutex・操作全体の期限を保証して呼ぶ。
// T=1状態は上位が所有する。失敗後は同じ操作内の追加アクセスを止める。
class CardUart final {
public:
	CardUart(MemoryAccess &memory, BootstrapWait &wait) : memory_(memory), wait_(wait) {}
	MemoryResult Result() const noexcept { return result_; }
	int WaitError() const noexcept { return wait_error_; }
	bool Present(bool &present)
	{
		Begin(); const auto irq = Read(0xfea01c);
		if (Good()) present = (irq & 0x400) == 0;
		return Good();
	}
	bool PowerDown() { Begin(); PowerDownImpl(); return Good(); }
	bool Reset()
	{
		Begin(); PowerDownImpl(); Pause(10);
		Write(0xfea01c, 0x3ff); Write(0xfea034, 1); Modify(0xfea004, 4, false);
		Write(0xfea014, 0); Write(0xfea000, 0); Write(0xfea000, 7);
		Modify(0xfea004, 8, true); Write(0xfea008, 0xba7f); Pause(2);
		Modify(0xfea004, 2, true); Write(0xfea03c, 0x3b);
		Write(0xfea020, 9600); Write(0xfea024, 0); Write(0xfea018, 0x1ff);
		Pause(1); Modify(0xfea004, 4, true); return Good();
	}
	bool SetBaudrate(unsigned rate)
	{
		Begin();
		if (rate != 9600 && rate != 19200 && rate != 38400) { result_ = MemoryResult::invalid_argument; return false; }
		Write(0xfea008, rate == 9600 ? 0xba7f : rate == 19200 ? 0x5d5d : 0x2eae);
		Write(0xfea010, rate == 9600 ? 0 : 0x10000);
		Write(0xfea020, rate == 9600 ? 9600 : 107);
		Write(0xfea024, rate == 9600 ? 0 : rate == 19200 ? 36559 : 0xffff);
		return Good();
	}
	bool ReadBytes(std::uint8_t *data, std::size_t capacity, std::size_t &count)
	{
		Begin(); count = 0;
		if (!data || !capacity || capacity > 259) { result_ = MemoryResult::invalid_argument; return false; }
		const auto available = Read(0xfea014) >> 16;
		const auto length = (std::min)(std::size_t(available), capacity);
		while (Good() && count < length) {
			const auto value = Read(0xfea02c);
			if (Good()) data[count++] = static_cast<std::uint8_t>(value);
		}
		return Good();
	}
	bool WriteBytes(const std::uint8_t *data, std::size_t size)
	{
		Begin();
		if (!data || !size || size > 259) { result_ = MemoryResult::invalid_argument; return false; }
		Write(0xfea03c, 0); Write(0xfea038, 0);
		for (std::size_t i = 0; i < size && Good(); ++i) Write(0xfea028, data[i]);
		Write(0xfea01c, 0x3ff); Write(0xfea010, size >= 256 ? 0x280 : 0x200);
		Write(0xfea00c, 1); return Good();
	}
private:
	void Begin() { result_ = MemoryResult::ok; wait_error_ = 0; }
	bool Good() const { return result_ == MemoryResult::ok && !wait_error_; }
	std::uint32_t Read(std::uint32_t address) { std::uint32_t value = 0; if (Good()) result_ = memory_.Read32(address, value); return value; }
	void Write(std::uint32_t address, std::uint32_t value) { if (Good()) result_ = memory_.Write32(address, value); }
	void Modify(std::uint32_t address, std::uint32_t bits, bool set) { const auto value = Read(address); Write(address, set ? value | bits : value & ~bits); }
	void Pause(unsigned ms) { if (Good()) wait_error_ = wait_.Pause(ms); }
	void PowerDownImpl() { Modify(0xfea004, 4, false); Write(0xfea014, 0); Modify(0xfea000, 4, false); Modify(0xfea004, 0xa, false); Write(0xfea018, 1); Write(0xfea034, 0); }
	MemoryAccess &memory_;
	BootstrapWait &wait_;
	MemoryResult result_ = MemoryResult::ok;
	int wait_error_ = 0;
};
} // namespace px4::torne
