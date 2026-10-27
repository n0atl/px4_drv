// Copyright 2026 Till0196
// Copyright (C) 2005-9 DiBcom, SA et al
// SPDX-License-Identifier: GPL-2.0-only
// Register access/identification adapted from coax dib8096/dib8000.go at
// 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405 (Linux media dib8000 origin).
#pragma once

#include "mb86h47_i2c.hpp"

namespace px4::torne {

class DiB8096Registers final {
public:
	explicit DiB8096Registers(MailboxI2c &bus) noexcept : bus_(bus) {}
	I2cResult Read(std::uint16_t number, std::uint16_t &value, MailboxI2c::Clock::time_point deadline)
	{
		const std::uint8_t address[]{static_cast<std::uint8_t>(number >> 8), static_cast<std::uint8_t>(number)};
		std::uint8_t data[2]{};
		const auto result = bus_.Transfer(0x12, address, 2, data, 2, deadline);
		if (result == I2cResult::ok)
			value = static_cast<std::uint16_t>((std::uint16_t(data[0]) << 8) | data[1]);
		return result;
	}
	I2cResult Write(std::uint16_t number, std::uint16_t value, MailboxI2c::Clock::time_point deadline)
	{
		const std::uint8_t data[]{static_cast<std::uint8_t>(number >> 8), static_cast<std::uint8_t>(number),
			static_cast<std::uint8_t>(value >> 8), static_cast<std::uint8_t>(value)};
		return bus_.Transfer(0x12, data, 4, nullptr, 0, deadline);
	}
	// 最初のvendor読み取りは電源投入直後に不安定な場合があり、元実装と同じく2回読む。
	I2cResult Identify(std::uint16_t &vendor, std::uint16_t &device, bool &recognized,
		MailboxI2c::Clock::time_point deadline)
	{
		recognized = false;
		vendor = device = 0;
		auto result = Read(896, vendor, deadline);
		if (result != I2cResult::ok) return result;
		result = Read(896, vendor, deadline);
		if (result != I2cResult::ok) return result;
		if (vendor != 0x01b3) return I2cResult::ok;
		result = Read(897, device, deadline);
		if (result == I2cResult::ok)
			recognized = device == 0x8000 || device == 0x8001 || device == 0x8002 || device == 0x8090;
		return result;
	}
private:
	MailboxI2c &bus_;
};

} // namespace px4::torne
