// SPDX-License-Identifier: GPL-2.0-only
#include "torne_capture.hpp"
#include "../src/DriverHost_PX4/torne/stream_framer.hpp"
#include <algorithm>
#include <array>
#include <vector>

bool CaptureFirstTs(HANDLE file, WINUSB_INTERFACE_HANDLE usb, HANDLE cancel,
	px4::torne::StreamEngine &engine, std::chrono::steady_clock::time_point deadline, TorneCaptureStats &stats, unsigned duration_ms,
	std::function<bool(const std::uint8_t *, std::size_t)> sink)
{
	stats = {};
	stats.prepare = engine.Prepare();
	if (stats.prepare != px4::torne::StreamResult::ok) {
		stats.stop = engine.Stop();
		return false;
	}
	if (!WinUsb_ResetPipe(usb, 0x81)) { stats.error = GetLastError(); stats.stop = engine.Stop(); return false; }
	const HANDLE completion = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!completion) { stats.error = GetLastError(); stats.stop = engine.Stop(); return false; }
	std::array<std::uint8_t, 6144> raw{};
	px4::torne::StreamFramer framer;
	std::vector<std::uint8_t> output;
	bool running = false;
	const auto begin = std::chrono::steady_clock::now();
	const auto end = duration_ms ? (std::min)(deadline, begin + std::chrono::milliseconds(duration_ms)) : deadline;
	auto last_read = begin;
	std::array<int, 8192> counters;
	counters.fill(-1);
	while (std::chrono::steady_clock::now() < end && (duration_ms || !stats.packets) && !stats.error) {
		if (!ResetEvent(completion)) { stats.error = GetLastError(); break; }
		OVERLAPPED operation{};
		operation.hEvent = completion;
		const BOOL submitted = WinUsb_ReadPipe(usb, 0x81, raw.data(), static_cast<ULONG>(raw.size()), nullptr, &operation);
		const DWORD submit_error = submitted ? 0 : GetLastError();
		if (!submitted && submit_error != ERROR_IO_PENDING) { stats.error = submit_error; break; }
		if (!running) {
			// 最初のUSB readを投入済みにしてからwriterを開始し、先頭データを受け止める。
			stats.start = engine.Start();
			running = stats.start == px4::torne::StreamResult::ok;
			if (!running) stats.error = ERROR_GEN_FAILURE;
		}
		if (!stats.error && !submitted) {
			const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(end - std::chrono::steady_clock::now()).count();
			const auto timeout = static_cast<DWORD>((std::clamp)(remaining, decltype(remaining)(0), decltype(remaining)(MAXDWORD - 1)));
			const HANDLE events[]{cancel, completion};
			const DWORD waited = WaitForMultipleObjects(2, events, FALSE, timeout);
			if (waited != WAIT_OBJECT_0 + 1)
				stats.error = waited == WAIT_TIMEOUT ? ERROR_TIMEOUT : waited == WAIT_FAILED ? GetLastError() : ERROR_OPERATION_ABORTED;
		}
		if (stats.error) CancelIoEx(file, &operation);
		ULONG transferred = 0;
		const BOOL collected = WinUsb_GetOverlappedResult(usb, &operation, &transferred, TRUE);
		if (!collected && !stats.error) stats.error = GetLastError();
		if (transferred > raw.size()) { stats.error = ERROR_INVALID_DATA; break; }
		stats.raw_bytes += transferred;
		++stats.reads;
		if (!stats.error && WaitForSingleObject(cancel, 0) == WAIT_OBJECT_0) stats.error = ERROR_OPERATION_ABORTED;
		// 計測区間の終端は正常停止。期限までに1パケットもない場合は下で失敗にする。
		if (stats.error == ERROR_TIMEOUT && duration_ms && stats.packets && end < deadline) stats.error = 0;
		if (!collected || stats.error) break;
		const auto now = std::chrono::steady_clock::now();
		stats.max_read_gap_ms = (std::max)(stats.max_read_gap_ms,
			std::chrono::duration_cast<std::chrono::milliseconds>(now - last_read).count());
		last_read = now;
		output.clear();
		framer.Feed(raw.data(), transferred, output);
		if (sink && !output.empty() && !sink(output.data(), output.size())) { stats.error = ERROR_BUFFER_OVERFLOW; break; }
		for (std::size_t offset = 0; offset < output.size(); offset += 188) {
			const auto *p = output.data() + offset;
			const unsigned pid = ((p[1] & 31) << 8) | p[2], afc = (p[3] >> 4) & 3;
			if (!afc || ((afc & 2) && (p[4] > 183 || ((afc & 1) && p[4] > 182)))) { ++stats.malformed; continue; }
			if (pid == 8191) continue;
			if ((p[1] & 0x80) || ((afc & 2) && p[4] && (p[5] & 0x80))) counters[pid] = -1;
			if (!(afc & 1) || (p[1] & 0x80)) continue;
			const int cc = p[3] & 15;
			// 同じCCの再送も含めた診断計数。放送側の重複とUSB欠落を区別する値ではない。
			if (counters[pid] >= 0 && cc != ((counters[pid] + 1) & 15)) ++stats.cc_anomalies;
			counters[pid] = cc;
		}
		stats.packets = framer.Packets();
		stats.tei = framer.TeiPackets();
		stats.discarded_bytes = framer.DiscardedBytes();
		if (stats.packets && stats.first_ts_ms < 0) stats.first_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
	}
	if (!stats.packets && !stats.error) stats.error = ERROR_TIMEOUT;
	if (duration_ms && end == deadline && !stats.error) stats.error = ERROR_TIMEOUT;
	// 全readの完了/cancelを回収済みなので、eventとバッファを安全に破棄できる。
	CloseHandle(completion);
	stats.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
	stats.stop = engine.Stop();
	return !stats.error && stats.packets && stats.stop == px4::torne::StreamResult::ok;
}
