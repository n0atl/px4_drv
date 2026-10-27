// SPDX-License-Identifier: GPL-2.0-only
#include <windows.h>
#include "IBonDriver2.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <string>

namespace {
using Clock = std::chrono::steady_clock;
struct Module {
	HMODULE handle = nullptr;
	~Module() { if (handle) FreeLibrary(handle); }
};
struct Driver {
	IBonDriver2 *value = nullptr;
	bool opened = false;
	~Driver() { if (value) { if (opened) value->CloseTuner(); value->Release(); } }
};
struct Metrics {
	std::uint64_t packets = 0, sync = 0, tei = 0, cc = 0, malformed = 0;
	std::uint64_t empty_reads = 0;
	std::uint64_t scrambled = 0;
	unsigned status_reads = 0;
	long long first_ms = -1;
	bool read_failed = false;
};

Metrics Measure(IBonDriver2 &driver, unsigned measure_ms, FILE *capture)
{
	Metrics result;
	std::array<int, 8192> previous;
	previous.fill(-1);
	const auto begin = Clock::now(), deadline = begin + std::chrono::milliseconds(measure_ms);
	auto next_status = begin + std::chrono::seconds(1);
	while (Clock::now() < deadline) {
		if (Clock::now() >= next_status) {
			driver.GetSignalLevel();
			++result.status_reads;
			next_status = Clock::now() + std::chrono::seconds(1);
		}
		driver.WaitTsStream(100);
		// 継続してバッファが届く状況でも観測期限でループを抜ける。
		while (Clock::now() < deadline && driver.GetReadyCount()) {
			BYTE *data = nullptr;
			DWORD size = 0, remaining = 0;
			// ポインター版で読み終えたcurrent bufferもGetReadyCountへ含まれ得る。
			// データなしのFALSEは待機へ戻り、無受信の成否は観測区間全体で判定する。
			if (!driver.GetTsStream(&data, &size, &remaining)) { ++result.empty_reads; break; }
			if (!data && size) { result.read_failed = true; return result; }
			if (!size) break;
			if (capture && std::fwrite(data,1,size,capture)!=size) { result.read_failed=true; return result; }
			if (size % 188) ++result.malformed;
			for (DWORD offset = 0; offset + 188 <= size; offset += 188) {
				const auto *packet = data + offset;
				++result.packets;
				if (packet[0] != 0x47) { ++result.sync; continue; }
				if (packet[3] & 0xc0) ++result.scrambled;
				if (result.first_ms < 0) result.first_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - begin).count();
				const unsigned pid = ((packet[1] & 31) << 8) | packet[2];
				const unsigned afc = (packet[3] >> 4) & 3;
				if (!afc || ((afc & 2) && (packet[4] > 183 || ((afc & 1) && packet[4] > 182)))) { ++result.malformed; continue; }
				if (packet[1] & 0x80) { ++result.tei; previous[pid] = -1; continue; }
				if ((afc & 2) && packet[4] && (packet[5] & 0x80)) previous[pid] = -1;
				if (pid == 8191 || !(afc & 1)) continue;
				const int counter = packet[3] & 15;
				// 同じCCの重複も異常候補として計数。USB欠落だけを意味する値ではない。
				if (previous[pid] >= 0 && counter != ((previous[pid] + 1) & 15)) ++result.cc;
				previous[pid] = counter;
			}
			if (!remaining) break;
		}
	}
	return result;
}
}

int wmain(int argc, wchar_t **argv)
{
	unsigned measure_ms = 3000;
	if (argc >= 6 && !wcscmp(argv[argc - 2], L"--measure-ms")) {
		wchar_t *end = nullptr;
		errno = 0;
		const auto value = wcstoul(argv[argc - 1], &end, 10);
		if (errno || argv[argc - 1][0] == L'-' || !end || *end || !value || value > 300000) return 2;
		measure_ms = static_cast<unsigned>(value);
		argc -= 2;
	}
	if (argc < 3 || (wcscmp(argv[2], L"--list") && wcscmp(argv[2], L"--receive"))) {
		std::fprintf(stderr, "Usage: torne_bondriver_probe.exe <absolute-dll-path> --list | --receive <channel-index>...\n");
		return 2;
	}
	const bool list = !wcscmp(argv[2], L"--list");
	if ((list && argc != 3) || (!list && (argc < 4 || argc > 20))) return 2;
	std::array<DWORD, 17> channels{};
	for (int i = 3; i < argc; ++i) {
		wchar_t *end = nullptr;
		errno = 0;
		const auto value = wcstoul(argv[i], &end, 10);
		if (errno || !argv[i][0] || argv[i][0] == L'-' || !end || *end || value > 255) return 2;
		channels[static_cast<std::size_t>(i - 3)] = value;
	}
	Module module;
	module.handle = LoadLibraryW(argv[1]);
	if (!module.handle) { std::fprintf(stderr, "FAIL: LoadLibrary error=%lu\n", GetLastError()); return 1; }
	const auto create = reinterpret_cast<IBonDriver *(*)()>(GetProcAddress(module.handle, "CreateBonDriver"));
	if (!create) { std::fprintf(stderr, "FAIL: missing CreateBonDriver\n"); return 1; }
	Driver driver;
	driver.value = static_cast<IBonDriver2 *>(create());
	if (!driver.value) { std::fprintf(stderr, "FAIL: CreateBonDriver\n"); return 1; }
	if (list) {
		for (DWORD index = 0; index < 256; ++index) {
			const auto *name = driver.value->EnumChannelName(0, index);
			if (!name) break;
			std::wprintf(L"space=0 index=%lu name=%ls\n", index, name);
		}
		std::wprintf(L"LIST ONLY: OpenTuner was not called.\n");
		return 0;
	}
	for (int i = 3; i < argc; ++i) {
		if (!driver.value->EnumChannelName(0, channels[static_cast<std::size_t>(i - 3)])) {
			std::fprintf(stderr, "FAIL: undefined channel index\n"); return 2;
		}
	}
	if (!driver.value->OpenTuner()) { std::fprintf(stderr, "FAIL: OpenTuner\n"); return 1; }
	driver.opened = true;
	std::printf("OPEN PASSED\n");
	std::fflush(stdout);
	bool passed = true;
	for (int i = 3; i < argc; ++i) {
		const auto index = channels[static_cast<std::size_t>(i - 3)];
		const auto begin = Clock::now();
		const bool tuned = !!driver.value->SetChannel(0, index);
		const auto tune_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - begin).count();
		if (!tuned) { passed = false; std::printf("FAIL: tune index=%lu tune_ms=%lld\n", index, tune_ms); continue; }
		driver.value->PurgeTsStream();
		// 診断時だけTSを保存する。環境変数未指定なら通常の計数だけを行う。
		FILE *capture=nullptr;
		wchar_t capture_dir[MAX_PATH]{};
		const auto capture_length=GetEnvironmentVariableW(L"TORNE_PROBE_CAPTURE_DIR",capture_dir,MAX_PATH);
		if(capture_length && capture_length<MAX_PATH){
			const auto capture_path=std::wstring(capture_dir)+L"\\channel-"+std::to_wstring(index+13)+L".ts";
			if(_wfopen_s(&capture,capture_path.c_str(),L"wb")){std::fprintf(stderr,"FAIL: capture open\n");return 1;}
		}
		const auto metrics = Measure(*driver.value, measure_ms, capture);
		if(capture)std::fclose(capture);
		const bool good = metrics.packets && !metrics.sync && !metrics.tei && !metrics.cc && !metrics.malformed && !metrics.read_failed;
		passed = passed && good;
		std::printf("index=%lu tune_ms=%lld first_ts_ms=%lld packets=%llu sync=%llu tei=%llu cc_anomalies=%llu malformed=%llu empty_reads=%llu scrambled=%llu status_reads=%u read_failed=%d signal=%.3f result=%s\n",
			index, tune_ms, metrics.first_ms, static_cast<unsigned long long>(metrics.packets),
			static_cast<unsigned long long>(metrics.sync), static_cast<unsigned long long>(metrics.tei),
			static_cast<unsigned long long>(metrics.cc), static_cast<unsigned long long>(metrics.malformed),
			static_cast<unsigned long long>(metrics.empty_reads), static_cast<unsigned long long>(metrics.scrambled), metrics.status_reads, metrics.read_failed, driver.value->GetSignalLevel(), good ? "PASS" : "FAIL");
		std::fflush(stdout);
	}
	driver.value->CloseTuner();
	driver.opened = false;
	std::printf("CLOSE RETURNED\nBONDRIVER TEST %s\n", passed ? "PASSED" : "FAILED");
	return passed ? 0 : 1;
}
