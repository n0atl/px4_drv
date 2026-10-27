// SPDX-License-Identifier: GPL-2.0-only
#include "winusb_control.hpp"

#include <algorithm>
#include <limits>

namespace px4::torne {

DWORD NativeControlIo::Submit(WINUSB_INTERFACE_HANDLE usb, const MemoryRequest &request,
	std::uint8_t *data, USHORT length, OVERLAPPED &operation) noexcept
{
	WINUSB_SETUP_PACKET setup{};
	setup.RequestType = request.type;
	setup.Request = request.request;
	setup.Value = request.value;
	setup.Index = request.index;
	setup.Length = length;
	return WinUsb_ControlTransfer(usb, setup, data, length, nullptr, &operation) ? ERROR_SUCCESS : GetLastError();
}

DWORD NativeControlIo::Collect(WINUSB_INTERFACE_HANDLE usb, OVERLAPPED &operation,
	ULONG &transferred, bool wait) noexcept
{
	return WinUsb_GetOverlappedResult(usb, &operation, &transferred, wait ? TRUE : FALSE) ? ERROR_SUCCESS : GetLastError();
}

DWORD NativeControlIo::Cancel(HANDLE file, OVERLAPPED &operation) noexcept
{
	// AbortPipeでは別要求まで取り消されるため、当該OVERLAPPEDだけを対象にする。
	return CancelIoEx(file, &operation) ? ERROR_SUCCESS : GetLastError();
}

WinUsbControl::WinUsbControl(HANDLE file, WINUSB_INTERFACE_HANDLE usb, HANDLE cancel, ControlIo &io) noexcept
	: file_(file), usb_(usb), cancel_(cancel), io_(io)
{}

int WinUsbControl::Pause(unsigned milliseconds)
{
	if (!cancel_)
		return ERROR_INVALID_HANDLE;
	const DWORD cancel_state = WaitForSingleObject(cancel_, 0);
	if (cancel_state == WAIT_OBJECT_0)
		return ERROR_OPERATION_ABORTED;
	if (cancel_state == WAIT_FAILED)
		return static_cast<int>(GetLastError());
	const auto now = Clock::now();
	if (now >= deadline_)
		return ERROR_TIMEOUT;
	const auto end = (std::min)(deadline_, now + std::chrono::milliseconds(milliseconds));
	const auto timeout = std::chrono::ceil<std::chrono::milliseconds>(end - now).count();
	const DWORD result = WaitForSingleObject(cancel_, static_cast<DWORD>((std::min)(timeout, static_cast<decltype(timeout)>(MAXDWORD - 1))));
	if (result == WAIT_OBJECT_0)
		return ERROR_OPERATION_ABORTED;
	if (result == WAIT_FAILED)
		return static_cast<int>(GetLastError());
	if (result != WAIT_TIMEOUT)
		return ERROR_INVALID_DATA;
	return Clock::now() >= deadline_ ? ERROR_TIMEOUT : ERROR_SUCCESS;
}

int WinUsbControl::Transfer(const MemoryRequest &request, std::uint8_t *data,
	std::size_t length, std::size_t &transferred)
{
	transferred = 0;
	if (!data || !length || length > 64 || file_ == INVALID_HANDLE_VALUE || !file_ || !usb_ || !cancel_)
		return ERROR_INVALID_PARAMETER;
	const DWORD cancel_state = WaitForSingleObject(cancel_, 0);
	if (cancel_state == WAIT_OBJECT_0)
		return ERROR_OPERATION_ABORTED;
	if (cancel_state == WAIT_FAILED)
		return static_cast<int>(GetLastError());
	if (Clock::now() >= deadline_)
		return ERROR_TIMEOUT;

	OVERLAPPED operation{};
	operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!operation.hEvent)
		return static_cast<int>(GetLastError());
	const DWORD submitted = io_.Submit(usb_, request, data, static_cast<USHORT>(length), operation);
	if (submitted != ERROR_SUCCESS && submitted != ERROR_IO_PENDING) {
		CloseHandle(operation.hEvent);
		return static_cast<int>(submitted);
	}

	DWORD reason = ERROR_SUCCESS;
	if (Clock::now() >= deadline_) {
		reason = ERROR_TIMEOUT;
	} else if (submitted == ERROR_IO_PENDING) {
		const auto remaining = deadline_ - Clock::now();
		// 切り捨てで期限より早く切らないよう、残時間をミリ秒へ切り上げる。
		const auto millis = std::chrono::ceil<std::chrono::milliseconds>(remaining).count();
		const auto timeout = static_cast<DWORD>(std::clamp<decltype(millis)>(millis, 0, MAXDWORD - 1));
		const HANDLE events[] = { cancel_, operation.hEvent };
		const DWORD result = WaitForMultipleObjects(2, events, FALSE, timeout);
		if (result == WAIT_OBJECT_0)
			reason = ERROR_OPERATION_ABORTED;
		else if (result == WAIT_TIMEOUT)
			reason = ERROR_TIMEOUT;
		else if (result != WAIT_OBJECT_0 + 1)
			reason = result == WAIT_FAILED ? GetLastError() : ERROR_INVALID_DATA;
	}
	if (!reason) {
		// 完了とcancelが競合した場合も、停止後の結果を新しい選局へ渡さない。
		const DWORD state = WaitForSingleObject(cancel_, 0);
		if (state == WAIT_OBJECT_0)
			reason = ERROR_OPERATION_ABORTED;
		else if (state == WAIT_FAILED)
			reason = GetLastError();
		else if (Clock::now() >= deadline_)
			reason = ERROR_TIMEOUT;
	}
	if (reason)
		io_.Cancel(file_, operation);

	ULONG actual = 0;
	// CancelIoExの成功は回収完了ではない。ERROR_NOT_FOUND（完了との競合）でも
	// Collectを終えるまでbuffer/OVERLAPPED/eventを解放できない。
	const DWORD collected = io_.Collect(usb_, operation, actual, true);
	CloseHandle(operation.hEvent);
	if (reason)
		return static_cast<int>(reason);
	if (collected)
		return static_cast<int>(collected);
	if (actual > length)
		return ERROR_INVALID_DATA;
	transferred = actual;
	return ERROR_SUCCESS;
}

} // namespace px4::torne
