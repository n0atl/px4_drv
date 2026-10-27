// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <array>
#include <cstdint>

namespace px4::torne {
// 選局epochごとの観測値。CC同値の重複も異常候補に含み、USB欠落数とは同義でない。
class TsQuality final {
public:
	TsQuality() { previous_.fill(-1); }
	void Feed(const std::uint8_t *packet) noexcept
	{
		++packets;
		if (packet[0] != 0x47) { ++malformed; return; }
		if (packet[3] & 0xc0) ++scrambled;
		const unsigned pid = ((packet[1] & 31) << 8) | packet[2], afc = (packet[3] >> 4) & 3;
		if (!afc || ((afc & 2) && (packet[4] > 183 || ((afc & 1) && packet[4] > 182)))) { ++malformed; return; }
		if (packet[1] & 0x80) { ++tei; previous_[pid] = -1; return; }
		if ((afc & 2) && packet[4] && (packet[5] & 0x80)) previous_[pid] = -1;
		if (pid == 8191 || !(afc & 1)) return;
		const int cc = packet[3] & 15;
		if (previous_[pid] >= 0 && cc != ((previous_[pid] + 1) & 15)) ++cc_anomalies;
		previous_[pid] = cc;
	}
	std::uint64_t packets = 0, tei = 0, malformed = 0, cc_anomalies = 0, scrambled = 0;
private:
	std::array<int, 8192> previous_;
};
}
