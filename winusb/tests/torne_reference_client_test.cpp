// SPDX-License-Identifier: GPL-2.0-only
#include "torne_reference_client.hpp"
#include <cstdlib>
#include <iostream>

static void Check(bool condition, const char *message)
{
	if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
class NoUsb final : public px4::torne::ControlTransport {
public:
	int Transfer(const px4::torne::MemoryRequest &, std::uint8_t *, std::size_t, std::size_t &) override
	{
		++calls;
		return ERROR_ACCESS_DENIED;
	}
	unsigned calls = 0;
};

int main()
{
	wchar_t path[32768]{};
	const DWORD size = GetModuleFileNameW(nullptr, path, 32768);
	Check(size && size < 32768, "executable path");
	std::wstring helper(path);
	helper = helper.substr(0, helper.find_last_of(L"\\/") + 1) + L"TorneFrontendDiagnostic.exe";
	NoUsb no_usb;
	px4::torne::MemoryAccess memory(no_usb);
	class Wait final : public px4::torne::BootstrapWait { int Pause(unsigned) override { return 0; } } wait;
	px4::torne::MailboxI2c bus(memory, wait);
	HANDLE cancel = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	Check(cancel != nullptr, "cancel event");
	{
		TorneReferenceClient client(bus, cancel);
		if (!client.Start(helper)) { std::cerr << client.Error() << '\n'; return 1; }
		std::vector<std::uint8_t> response;
		for (unsigned i = 0; i < 3; ++i) {
			const bool result = client.Command(0x13, {}, response, std::chrono::steady_clock::now() + std::chrono::seconds(5));
			if (result || client.Error() != "frontend is not initialized") { std::cerr << client.Error() << '\n'; return 1; }
		}
		Check(!client.Command(0x13, {}, response, std::chrono::steady_clock::now() - std::chrono::seconds(1)), "expired IPC deadline");
		SetEvent(cancel);
		Check(!client.Command(0x13, {}, response, std::chrono::steady_clock::now() + std::chrono::seconds(5)), "IPC cancelled before submission");
		Check(no_usb.calls == 0 && client.I2cRequests() == 0, "IPC tests must never touch USB");
	}
	CloseHandle(cancel);
	std::cout << "PASS: native/reference IPC (no USB access)\n";
}
