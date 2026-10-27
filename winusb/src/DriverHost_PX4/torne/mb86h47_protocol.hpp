// SPDX-License-Identifier: GPL-2.0-only
// Protocol reference: Till0196/coax, commit 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405,
// internal/coaxd/usb/bridge/mb86h47/mb86h47.go (Copyright 2026 Till0196).
// GPLv2 branch selected. C++ adaptation adds exact-length and range checks.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace px4::torne {

struct MemoryRequest {
	std::uint8_t type;
	std::uint8_t request;
	std::uint16_t value;
	std::uint16_t index;
};

// アドレス拡張後のメモリ要求専用。拡張前の起動処理は別途必要となる。
constexpr MemoryRequest MakeMemoryRequest(std::uint32_t address, bool read) noexcept
{
	return { static_cast<std::uint8_t>(read ? 0xc0 : 0x40), 0xbc,
		static_cast<std::uint16_t>(((address >> 8) & 0xff00) | (address >> 24)),
		static_cast<std::uint16_t>(address) };
}

// 将来のWinUSB層はcancelと絶対期限を実装し、OSエラーをこの境界まで保持する。
class ControlTransport {
public:
	virtual ~ControlTransport() = default;
	virtual int Transfer(const MemoryRequest &request, std::uint8_t *data,
		std::size_t length, std::size_t &transferred) = 0;
};

enum class MemoryResult { ok, invalid_argument, transport_error, short_transfer };

class MemoryAccess final {
public:
	explicit MemoryAccess(ControlTransport &transport) : transport_(transport) {}
	int LastTransportError() const noexcept { return last_error_; }

	MemoryResult Read32(std::uint32_t address, std::uint32_t &value)
	{
		std::array<std::uint8_t, 4> bytes{};
		const auto result = Transfer(address, true, bytes.data(), bytes.size());
		if (result == MemoryResult::ok)
			value = (std::uint32_t(bytes[0]) << 24) | (std::uint32_t(bytes[1]) << 16) |
				(std::uint32_t(bytes[2]) << 8) | bytes[3];
		return result;
	}

	MemoryResult Write32(std::uint32_t address, std::uint32_t value)
	{
		std::array<std::uint8_t, 4> bytes{ static_cast<std::uint8_t>(value >> 24),
			static_cast<std::uint8_t>(value >> 16), static_cast<std::uint8_t>(value >> 8),
			static_cast<std::uint8_t>(value) };
		return Transfer(address, false, bytes.data(), bytes.size());
	}

	MemoryResult Write16(std::uint32_t address, std::uint16_t value)
	{
		std::uint32_t previous = 0;
		const auto result = Read32(address, previous);
		if (result != MemoryResult::ok)
			return result;
		return Write32(address, (std::uint32_t(value) << 16) | (previous & 0xffff));
	}

	MemoryResult Transfer(std::uint32_t address, bool read, std::uint8_t *data, std::size_t size)
	{
		last_error_ = 0;
		if (!data || !size || size > 64 || address > UINT32_MAX - (size - 1))
			return MemoryResult::invalid_argument;
		std::size_t transferred = 0;
		last_error_ = transport_.Transfer(MakeMemoryRequest(address, read), data, size, transferred);
		if (last_error_)
			return MemoryResult::transport_error;
		return transferred == size ? MemoryResult::ok : MemoryResult::short_transfer;
	}

private:
	ControlTransport &transport_;
	int last_error_ = 0;
};

} // namespace px4::torne
