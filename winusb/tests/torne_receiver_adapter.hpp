// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "torne_reference_client.hpp"
#include "torne_capture.hpp"

// 初期化済みの1台を借りる実機統合試験。通常のDriverHostへの機種登録は行わない。
bool TestTorneReceiver(HANDLE file, WINUSB_INTERFACE_HANDLE usb,
	px4::torne::MemoryAccess &memory, px4::torne::Bootstrap &bootstrap,
	TorneReferenceClient &reference, std::chrono::steady_clock::time_point deadline,
	const std::vector<unsigned> &channels, unsigned observation_ms, bool inject_no_ts);
