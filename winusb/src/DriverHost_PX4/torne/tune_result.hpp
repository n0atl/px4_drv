// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstddef>
#include <cstdint>

namespace px4::torne {
// 旧helperの空応答や未知の形式は早期失敗にせず、従来のlock待ちを維持する。
inline bool IsConfirmedSearchFailure(const std::uint8_t *data, std::size_t size) noexcept
{
	return data && size == 1 && data[0] == 1;
}
}
