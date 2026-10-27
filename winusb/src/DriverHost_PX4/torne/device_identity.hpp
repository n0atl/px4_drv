// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace px4::torne {
// シリアル番号を持たないtorneでは、同じUSBポートのinterface pathを選択キーにする。
// 短いIDは表示/指定用。衝突時は曖昧として拒否し、認証や製造番号には使わない。
inline std::string InterfaceId(std::wstring_view path)
{
	std::uint64_t hash = 14695981039346656037ULL;
	for (auto character : path) {
		if (character >= L'A' && character <= L'Z') character = static_cast<wchar_t>(character + (L'a' - L'A'));
		hash ^= static_cast<std::uint16_t>(character);
		hash *= 1099511628211ULL;
	}
	std::string id(16, '0');
	constexpr char digits[] = "0123456789abcdef";
	for (unsigned i = 0; i < 16; ++i) { id[15 - i] = digits[hash & 15]; hash >>= 4; }
	return id;
}

inline bool NormalizeInterfaceId(std::string_view value, std::string &normalized)
{
	normalized.clear();
	if (value.size() != 16) return false;
	for (char digit : value) {
		if (digit >= 'A' && digit <= 'F') digit = static_cast<char>(digit + ('a' - 'A'));
		if (!((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f'))) { normalized.clear(); return false; }
		normalized += digit;
	}
	return true;
}

enum class SelectionResult { ok, invalid_id, not_found, ambiguous };

inline SelectionResult SelectInterface(const std::vector<std::wstring> &paths,
	std::string_view requested_id, std::size_t &selected)
{
	selected = paths.size();
	std::string id;
	if (!requested_id.empty() && !NormalizeInterfaceId(requested_id, id)) return SelectionResult::invalid_id;
	for (std::size_t i = 0; i < paths.size(); ++i) {
		if (paths[i].empty() || (!id.empty() && InterfaceId(paths[i]) != id)) continue;
		if (selected != paths.size()) { selected = paths.size(); return SelectionResult::ambiguous; }
		selected = i;
	}
	return selected == paths.size() ? SelectionResult::not_found : SelectionResult::ok;
}
} // namespace px4::torne
