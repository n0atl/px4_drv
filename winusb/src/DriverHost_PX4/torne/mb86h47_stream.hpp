// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
// C++ adaptation of coax mb86h47/stream.go at
// 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405; GPLv2 branch selected.
#pragma once
#include "mb86h47_bootstrap.hpp"

namespace px4::torne {
enum class StreamResult { ok, invalid_state, memory_error, endpoint_reset_failed };

// USB読取りの投入・回収と復調器出力切替は所有者の責務。
// Prepare完了後にreaderを投入し、parallel出力を確認してStartへ進む。
class StreamEngine final {
public:
	StreamEngine(MemoryAccess &memory, const Bootstrap &bootstrap) noexcept : memory_(memory), bootstrap_(bootstrap) {}
	StreamResult Prepare();
	StreamResult Start();
	StreamResult Stop();
	std::uint32_t LastAddress() const noexcept { return address_; }
	MemoryResult LastMemoryResult() const noexcept { return memory_result_; }
private:
	std::uint32_t Read(std::uint32_t address);
	void Write(std::uint32_t address, std::uint32_t value);
	void Block(std::uint32_t address, std::uint8_t *data, std::size_t size, bool read);
	void LoadCore0();
	MemoryAccess &memory_;
	const Bootstrap &bootstrap_;
	StreamResult result_ = StreamResult::ok;
	MemoryResult memory_result_ = MemoryResult::ok;
	std::uint32_t address_ = 0;
	std::uint64_t generation_ = 0;
	bool prepared_ = false, running_ = false;
};
}
