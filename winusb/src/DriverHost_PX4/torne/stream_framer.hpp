// SPDX-License-Identifier: GPL-2.0-only
// Format reference: Till0196/coax, commit 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405,
// internal/coaxd/usb/bridge/mb86h47/stream.go (Copyright 2026 Till0196).
// GPLv2 branch selected. C++ adaptation uses three complete units to acquire sync.
#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "../../../../driver/ts_sync.h"

namespace px4::torne {

// 個体ごとの受信workerのみが所有する。選局前にI/Oを回収してResetする。
class StreamFramer final {
public:
	static constexpr std::size_t unit_size = 192;
	static constexpr std::size_t packet_size = 188;
	explicit StreamFramer(bool accept_error_candidates=false) noexcept : accept_error_candidates_(accept_error_candidates) {}

	void Reset() noexcept
	{
		pending_.clear();
		locked_ = false;
		discarded_ = packets_ = tei_packets_ = 0;
	}

	void Feed(const std::uint8_t *data, std::size_t size, std::vector<std::uint8_t> &output)
	{
		if (!size)
			return;
		if (!data)
			throw std::invalid_argument("StreamFramer: null input");
		pending_.insert(pending_.end(), data, data + size);
		std::size_t offset = 0;
		while (pending_.size() - offset >= unit_size) {
			if (!locked_) {
				// 単発の0x47やTEI付きデータを境界獲得の根拠にするとpayloadへ誤同期し得る。
				const std::size_t candidates=accept_error_candidates_?8:3;
				if (pending_.size() - offset < candidates * unit_size)
					break;
				bool valid=true;
				for(std::size_t i=0;i<candidates;++i)valid=valid && Candidate(offset+i*unit_size);
				if (!valid) {
					++offset;
					++discarded_;
					continue;
				}
				locked_ = true;
			}
			const auto *packet = pending_.data() + offset + 4;
			if (!px4_ts_has_plain_sync(packet[0])) {
				locked_ = false;
				continue;
			}
			// 同期済みのTEIは上位のエラー計数に残す。破棄して品質を良く見せない。
			if (packet[1] & 0x80)
				++tei_packets_;
			output.insert(output.end(), packet, packet + packet_size);
			++packets_;
			offset += unit_size;
		}
		pending_.erase(pending_.begin(), pending_.begin() + offset);
	}

	std::size_t PendingSize() const noexcept { return pending_.size(); }
	std::uint64_t DiscardedBytes() const noexcept { return discarded_; }
	std::uint64_t Packets() const noexcept { return packets_; }
	std::uint64_t TeiPackets() const noexcept { return tei_packets_; }

private:
	bool Candidate(std::size_t offset) const noexcept
	{
		const auto *packet = pending_.data() + offset + 4;
		return px4_ts_has_plain_sync(packet[0]) && (accept_error_candidates_ || !(packet[1] & 0x80)) && (packet[3] & 0x30);
	}

	std::vector<std::uint8_t> pending_;
	bool locked_ = false;
	// 未訂正TS出力の診断時だけTEIを許容し、8個の192byte周期と有効AFCで誤同期を抑える。
	const bool accept_error_candidates_;
	std::uint64_t discarded_ = 0;
	std::uint64_t packets_ = 0;
	std::uint64_t tei_packets_ = 0;
};

} // namespace px4::torne
