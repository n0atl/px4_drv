// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
// C++ adaptation of coax mb86h47/i2c.go at
// 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405; GPLv2 branch selected.
#pragma once

#include "mb86h47_bootstrap.hpp"

namespace px4::torne {

enum class I2cResult {
	ok, invalid_argument, not_ready, memory_error, timeout, malformed_answer,
	bus_error, address_nack, data_nack, stuck
};

// 初期統合用の同期mailbox経路。ring/commandと併用すると順序保証を失うため、
// 同一個体で別のI2C経路を使用する場合は先に全要求をdrainする必要がある。
class MailboxI2c final {
public:
	using Clock = std::chrono::steady_clock;
	MailboxI2c(MemoryAccess &memory, BootstrapWait &clock) noexcept : memory_(memory), clock_(clock) {}
	bool BeginAfterBootstrap(const Bootstrap &bootstrap) noexcept;
	void Invalidate() noexcept { ready_ = false; }
	I2cResult Transfer(std::uint8_t address, const std::uint8_t *write, std::size_t write_size,
		std::uint8_t *read, std::size_t read_size, Clock::time_point deadline);
	MemoryResult LastMemoryResult() const noexcept { return memory_result_; }
	int LastSystemError() const noexcept { return memory_.LastTransportError(); }

private:
	MemoryAccess &memory_;
	BootstrapWait &clock_;
	std::uint8_t sequence_ = 0;
	bool ready_ = false;
	MemoryResult memory_result_ = MemoryResult::ok;
	const Bootstrap *bootstrap_ = nullptr;
	std::uint64_t generation_ = 0;
};

} // namespace px4::torne
