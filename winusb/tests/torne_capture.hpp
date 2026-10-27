// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <windows.h>
#include <winusb.h>
#include "../src/DriverHost_PX4/torne/mb86h47_stream.hpp"

struct TorneCaptureStats {
	DWORD error = 0;
	px4::torne::StreamResult prepare = px4::torne::StreamResult::invalid_state;
	px4::torne::StreamResult start = px4::torne::StreamResult::invalid_state;
	px4::torne::StreamResult stop = px4::torne::StreamResult::invalid_state;
	std::uint64_t raw_bytes = 0, packets = 0, tei = 0, discarded_bytes = 0, reads = 0;
	long long first_ts_ms = -1;
	std::uint64_t cc_anomalies = 0, malformed = 0;
	long long elapsed_ms = 0, max_read_gap_ms = 0;
};
// 診断専用の逐次read。duration_ms=0は先頭TS、それ以外は有限時間の品質計測。
// 製品用の並列受信workerや長時間安定性の保証としては扱わない。
bool CaptureFirstTs(HANDLE file, WINUSB_INTERFACE_HANDLE usb, HANDLE cancel,
	px4::torne::StreamEngine &engine, std::chrono::steady_clock::time_point deadline, TorneCaptureStats &stats,
	unsigned duration_ms = 0,
	std::function<bool(const std::uint8_t *, std::size_t)> output = {});
