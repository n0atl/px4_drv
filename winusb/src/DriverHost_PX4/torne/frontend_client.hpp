// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include "mb86h47_i2c.hpp"

// Experimental adapter: the child owns demodulator state, never a USB handle.
class TorneFrontendClient final {
public:
	TorneFrontendClient(px4::torne::MailboxI2c &bus, HANDLE cancel) noexcept : bus_(bus), cancel_(cancel) {}
	~TorneFrontendClient();
	TorneFrontendClient(const TorneFrontendClient &) = delete;
	TorneFrontendClient &operator=(const TorneFrontendClient &) = delete;
	bool Start(const std::wstring &executable);
	bool Command(std::uint8_t kind, const std::vector<std::uint8_t> &body,
		std::vector<std::uint8_t> &response, std::chrono::steady_clock::time_point deadline);
	const std::string &Error() const noexcept { return error_; }
	std::uint64_t I2cRequests() const noexcept { return requests_; }
private:
	bool Exchange(void *data, std::size_t size, bool write, std::chrono::steady_clock::time_point deadline);
	bool Send(std::uint8_t kind, const std::vector<std::uint8_t> &body, std::chrono::steady_clock::time_point deadline);
	bool Fail(const std::string &message);
	px4::torne::MailboxI2c &bus_;
	HANDLE cancel_;
	HANDLE pipe_ = INVALID_HANDLE_VALUE;
	HANDLE process_ = nullptr;
	std::string error_;
	std::uint64_t requests_ = 0;
};
