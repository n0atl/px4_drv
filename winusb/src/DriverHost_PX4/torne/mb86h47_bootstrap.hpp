// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
// C++ adaptation of coax mb86h47.go, addressing.go and i2c.go at
// 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405; GPLv2 branch selected.
#pragma once

#include <chrono>
#include "mb86h47_protocol.hpp"

namespace px4::torne {

class BootstrapWait {
public:
	virtual ~BootstrapWait() = default;
	// 初期化全体と同じ絶対期限・停止eventに従う。0=完了、それ以外=OSエラー。
	virtual int Pause(unsigned milliseconds) = 0;
	virtual std::chrono::steady_clock::time_point Now() const noexcept { return std::chrono::steady_clock::now(); }
};

enum class BootstrapResult {
	ok, memory_error, wait_error, not_power_up_state,
	unknown_handler, verification_failed, core1_not_running, invalid_seed, not_initialized
};

// 外部所有者が個体を排他し、制御workerを直列化してから呼ぶ。
// Initializeはアドレス拡張・クロック・trap・I2C coreまで。復調器初期化は別層。
class Bootstrap final {
public:
	Bootstrap(MemoryAccess &memory, BootstrapWait &wait) noexcept : memory_(memory), wait_(wait) {}
	BootstrapResult Initialize(std::uint32_t command_seed);
	BootstrapResult ResetDemodulator();
	BootstrapResult Result() const noexcept { return result_; }
	MemoryResult LastMemoryResult() const noexcept { return memory_result_; }
	int LastSystemError() const noexcept { return system_error_; }
	std::uint32_t LastAddress() const noexcept { return last_address_; }
	bool IsInitialized() const noexcept { return initialized_; }
	std::uint64_t Generation() const noexcept { return generation_; }

private:
	void ClearError() noexcept;
	std::uint32_t Read(std::uint32_t address);
	void Write(std::uint32_t address, std::uint32_t value);
	void Write16(std::uint32_t address, std::uint16_t value);
	void Block(std::uint32_t address, std::uint8_t *data, std::size_t size, bool read);
	void Pause(unsigned milliseconds);
	void Widen();
	void Clocks();
	void Traps();
	void I2cInit();
	void Load(std::uint32_t base, const std::uint32_t *program, std::size_t count);
	void Core1(std::uint32_t seed);

	MemoryAccess &memory_;
	BootstrapWait &wait_;
	BootstrapResult result_ = BootstrapResult::ok;
	MemoryResult memory_result_ = MemoryResult::ok;
	int system_error_ = 0;
	std::uint32_t last_address_ = 0;
	bool initialized_ = false;
	std::uint64_t generation_ = 0;
};

} // namespace px4::torne
