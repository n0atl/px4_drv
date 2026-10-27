// SPDX-License-Identifier: GPL-2.0-only
#include "torne_reference_client.hpp"

#include <algorithm>
#include <array>
#include <atomic>

namespace {
struct Handle final {
	explicit Handle(HANDLE handle_value = nullptr) : value(handle_value) {}
	~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;
	HANDLE value;
};
std::atomic<unsigned> next_pipe{0};
std::uint32_t Little(const std::uint8_t *p) noexcept
{
	return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
}

TorneReferenceClient::~TorneReferenceClient()
{
	if (pipe_ != INVALID_HANDLE_VALUE) CloseHandle(pipe_);
	if (process_) {
		// このオブジェクトが起動した子だけを終了対象とし、既存coax/DriverHostへ触れない。
		if (WaitForSingleObject(process_, 1000) == WAIT_TIMEOUT) {
			TerminateProcess(process_, ERROR_OPERATION_ABORTED);
			WaitForSingleObject(process_, 1000);
		}
		CloseHandle(process_);
	}
}

bool TorneReferenceClient::Fail(const std::string &message)
{
	error_ = message;
	return false;
}

bool TorneReferenceClient::Start(const std::wstring &executable)
{
	if (process_ || pipe_ != INVALID_HANDLE_VALUE)
		return Fail("reference process already started");
	const auto name = L"\\\\.\\pipe\\px4_torne_reference_" + std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(++next_pipe);
	pipe_ = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 65536, 65536, 0, nullptr);
	if (pipe_ == INVALID_HANDLE_VALUE)
		return Fail("CreateNamedPipe error=" + std::to_string(GetLastError()));
	SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
	Handle child(CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, &attributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	if (child.value == INVALID_HANDLE_VALUE)
		return Fail("open child pipe error=" + std::to_string(GetLastError()));
	Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
	if (!event.value) return Fail("create connect event failed");
	OVERLAPPED connected{};
	connected.hEvent = event.value;
	if (!ConnectNamedPipe(pipe_, &connected)) {
		const DWORD error = GetLastError();
		if (error != ERROR_PIPE_CONNECTED) {
			if (error == ERROR_IO_PENDING) {
				CancelIoEx(pipe_, &connected);
				DWORD ignored = 0;
				GetOverlappedResult(pipe_, &connected, &ignored, TRUE);
			}
			return Fail("connect reference pipe error=" + std::to_string(error));
		}
	}
	Handle stderr_copy;
	if (!DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_ERROR_HANDLE), GetCurrentProcess(),
		&stderr_copy.value, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
		stderr_copy.value = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes, OPEN_EXISTING, 0, nullptr);
		if (stderr_copy.value == INVALID_HANDLE_VALUE) return Fail("create child stderr failed");
	}
	SIZE_T required = 0;
	InitializeProcThreadAttributeList(nullptr, 1, 0, &required);
	std::vector<std::uint8_t> storage(required);
	auto *list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
	if (!InitializeProcThreadAttributeList(list, 1, 0, &required)) return Fail("initialize child handle list failed");
	const HANDLE inherited[]{child.value, stderr_copy.value};
	if (!UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, const_cast<HANDLE *>(inherited), sizeof(inherited), nullptr, nullptr)) {
		DeleteProcThreadAttributeList(list);
		return Fail("configure child handle list failed");
	}
	STARTUPINFOEXW startup{};
	startup.StartupInfo.cb = sizeof(startup);
	startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
	startup.StartupInfo.hStdInput = startup.StartupInfo.hStdOutput = child.value;
	startup.StartupInfo.hStdError = stderr_copy.value;
	startup.lpAttributeList = list;
	PROCESS_INFORMATION process{};
	auto command = L"\"" + executable + L"\" --stdio-diagnostic-v1";
	const BOOL created = CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &startup.StartupInfo, &process);
	const DWORD creation_error = created ? 0 : GetLastError();
	DeleteProcThreadAttributeList(list);
	if (!created) return Fail("CreateProcess error=" + std::to_string(creation_error));
	CloseHandle(process.hThread);
	process_ = process.hProcess;
	return true;
}

bool TorneReferenceClient::Exchange(void *data, std::size_t size, bool write, std::chrono::steady_clock::time_point deadline)
{
	auto *bytes = static_cast<std::uint8_t *>(data);
	while (size) {
		const auto now = std::chrono::steady_clock::now();
		if (now >= deadline) return Fail("reference deadline expired");
		if (WaitForSingleObject(cancel_, 0) == WAIT_OBJECT_0) return Fail("reference cancelled");
		Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
		if (!event.value) return Fail("create I/O event failed");
		OVERLAPPED operation{};
		operation.hEvent = event.value;
		const DWORD chunk = static_cast<DWORD>((std::min)(size, std::size_t(4096)));
		const BOOL submitted = write ? WriteFile(pipe_, bytes, chunk, nullptr, &operation) : ReadFile(pipe_, bytes, chunk, nullptr, &operation);
		const DWORD submit_error = submitted ? 0 : GetLastError();
		if (!submitted && submit_error != ERROR_IO_PENDING)
			return Fail("reference pipe submit error=" + std::to_string(submit_error));
		DWORD reason = 0;
		if (!submitted) {
			const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
			const DWORD timeout = static_cast<DWORD>((std::clamp)(remaining, decltype(remaining)(0), decltype(remaining)(MAXDWORD - 1)));
			const HANDLE events[]{cancel_, event.value, process_};
			const DWORD waited = WaitForMultipleObjects(3, events, FALSE, timeout);
			if (waited != WAIT_OBJECT_0 + 1)
				reason = waited == WAIT_TIMEOUT ? ERROR_TIMEOUT : waited == WAIT_FAILED ? GetLastError() : ERROR_OPERATION_ABORTED;
		}
		if (reason) CancelIoEx(pipe_, &operation);
		DWORD actual = 0;
		const BOOL collected = GetOverlappedResult(pipe_, &operation, &actual, TRUE);
		if (reason) return Fail("reference wait error=" + std::to_string(reason));
		if (!collected) return Fail("reference pipe completion error=" + std::to_string(GetLastError()));
		if (!actual || actual > chunk) return Fail("reference invalid transfer length");
		bytes += actual;
		size -= actual;
	}
	return true;
}

bool TorneReferenceClient::Send(std::uint8_t kind, const std::vector<std::uint8_t> &body, std::chrono::steady_clock::time_point deadline)
{
	if (body.size() > 4096) return Fail("oversized outgoing reference frame");
	std::array<std::uint8_t, 8> header{kind};
	for (unsigned i = 0; i < 4; ++i) header[4 + i] = static_cast<std::uint8_t>(body.size() >> (8 * i));
	if (!Exchange(header.data(), header.size(), true, deadline)) return false;
	auto copy = body;
	return copy.empty() || Exchange(copy.data(), copy.size(), true, deadline);
}

bool TorneReferenceClient::Command(std::uint8_t kind, const std::vector<std::uint8_t> &body,
	std::vector<std::uint8_t> &response, std::chrono::steady_clock::time_point deadline)
{
	error_.clear();
	response.clear();
	if (!process_) return Fail("reference process not started");
	if (!Send(kind, body, deadline)) return false;
	for (;;) {
		std::array<std::uint8_t, 8> header{};
		if (!Exchange(header.data(), header.size(), false, deadline)) return false;
		const auto size = Little(header.data() + 4);
		if (size > 4096) return Fail("oversized incoming reference frame");
		std::vector<std::uint8_t> payload(size);
		if (size && !Exchange(payload.data(), size, false, deadline)) return false;
		if (header[0] == 0xff) return Fail(std::string(payload.begin(), payload.end()));
		if (header[0] == 0x80) { response = std::move(payload); return true; }
		if (header[0] != 1 || header[2] > 32 || header[3] > 32 || payload.size() != header[2] || (!header[2] && !header[3]))
			return Fail("invalid reference I2C request");
		++requests_;
		std::vector<std::uint8_t> read(header[3]);
		const auto result = bus_.Transfer(header[1], payload.data(), payload.size(), read.data(), read.size(), deadline);
		if (result != px4::torne::I2cResult::ok) {
			const auto message = "I2C result=" + std::to_string(static_cast<unsigned>(result)) + " memory=" +
				std::to_string(static_cast<unsigned>(bus_.LastMemoryResult())) + " os=" + std::to_string(bus_.LastSystemError());
			if (!Send(0xff, std::vector<std::uint8_t>(message.begin(), message.end()), deadline)) return false;
		} else if (!Send(0x81, read, deadline)) return false;
	}
}
