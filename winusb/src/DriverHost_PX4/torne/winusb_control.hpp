// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include <chrono>
#include <windows.h>
#include <winusb.h>

#include "mb86h47_protocol.hpp"
#include "mb86h47_bootstrap.hpp"

namespace px4::torne {

// I/O完了後までOVERLAPPEDとbufferを保持する契約。模擬APIも同じ寿命条件を使う。
class ControlIo {
public:
	virtual ~ControlIo() = default;
	virtual DWORD Submit(WINUSB_INTERFACE_HANDLE usb, const MemoryRequest &request,
		std::uint8_t *data, USHORT length, OVERLAPPED &operation) noexcept = 0;
	virtual DWORD Collect(WINUSB_INTERFACE_HANDLE usb, OVERLAPPED &operation,
		ULONG &transferred, bool wait) noexcept = 0;
	virtual DWORD Cancel(HANDLE file, OVERLAPPED &operation) noexcept = 0;
};

class NativeControlIo final : public ControlIo {
public:
	DWORD Submit(WINUSB_INTERFACE_HANDLE usb, const MemoryRequest &request,
		std::uint8_t *data, USHORT length, OVERLAPPED &operation) noexcept override;
	DWORD Collect(WINUSB_INTERFACE_HANDLE usb, OVERLAPPED &operation,
		ULONG &transferred, bool wait) noexcept override;
	DWORD Cancel(HANDLE file, OVERLAPPED &operation) noexcept override;
};

// この個体の制御workerだけが呼ぶ。usb/file/cancelは全Transferの回収まで生存させる。
// Beginは各レジスター操作ではなく、初期化・選局などの操作全体に一度だけ適用する。
class WinUsbControl final : public ControlTransport, public BootstrapWait {
public:
	using Clock = std::chrono::steady_clock;
	WinUsbControl(HANDLE file, WINUSB_INTERFACE_HANDLE usb, HANDLE cancel, ControlIo &io) noexcept;
	void Begin(Clock::time_point deadline) noexcept { deadline_ = deadline; }
	int Transfer(const MemoryRequest &request, std::uint8_t *data,
		std::size_t length, std::size_t &transferred) override;
	int Pause(unsigned milliseconds) override;

private:
	HANDLE file_;
	WINUSB_INTERFACE_HANDLE usb_;
	HANDLE cancel_;
	ControlIo &io_;
	Clock::time_point deadline_ = (Clock::time_point::min)();
};

} // namespace px4::torne
