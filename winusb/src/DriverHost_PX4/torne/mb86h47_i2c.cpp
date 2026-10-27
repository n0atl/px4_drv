// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
#include "mb86h47_i2c.hpp"

#include <algorithm>
#include <array>

namespace px4::torne {

bool MailboxI2c::BeginAfterBootstrap(const Bootstrap &bootstrap) noexcept
{
	if (bootstrap_ == &bootstrap && generation_ == bootstrap.Generation())
		return false;
	ready_ = bootstrap.IsInitialized() && bootstrap.Result() == BootstrapResult::ok;
	if (!ready_)
		return false;
	bootstrap_ = &bootstrap;
	generation_ = bootstrap.Generation();
	sequence_ = 0;
	memory_result_ = MemoryResult::ok;
	return ready_;
}

I2cResult MailboxI2c::Transfer(std::uint8_t address, const std::uint8_t *write, std::size_t write_size,
	std::uint8_t *read, std::size_t read_size, Clock::time_point deadline)
{
	memory_result_ = MemoryResult::ok;
	if ((address & 1) || write_size > 32 || read_size > 32 || (!write_size && !read_size) ||
		(write_size && !write) || (read_size && !read))
		return I2cResult::invalid_argument;
	if (!ready_ || !bootstrap_ || !bootstrap_->IsInitialized() || bootstrap_->Generation() != generation_) {
		ready_ = false;
		return I2cResult::not_ready;
	}
	if (clock_.Now() >= deadline)
		return I2cResult::timeout;
	++sequence_;
	if (!sequence_)
		sequence_ = 1;
	auto failed = [&](I2cResult result) {
		// core1側で完了したか不明な要求の連番を再利用すると、遅延応答へ誤一致し得る。
		ready_ = false;
		return result;
	};
	for (std::size_t offset = 0; offset < write_size; offset += 4) {
		if (clock_.Now() >= deadline)
			return failed(I2cResult::timeout);
		std::uint32_t word = 0;
		for (unsigned byte = 0; byte < 4; ++byte) {
			word <<= 8;
			if (offset + byte < write_size)
				word |= write[offset + byte];
		}
		memory_result_ = memory_.Write32(0x17a3f04 + static_cast<std::uint32_t>(offset), word);
		if (memory_result_ != MemoryResult::ok)
			return failed(I2cResult::memory_error);
	}
	if (clock_.Now() >= deadline)
		return failed(I2cResult::timeout);
	const auto command = (std::uint32_t(sequence_) << 24) | (std::uint32_t(address) << 16) |
		(static_cast<std::uint32_t>(write_size) << 8) | static_cast<std::uint32_t>(read_size);
	// データを配置し終えてからsequenceを含むコマンドを最後に書く。
	memory_result_ = memory_.Write32(0x17a3f00, command);
	if (memory_result_ != MemoryResult::ok)
		return failed(I2cResult::memory_error);
	std::array<std::uint8_t, 36> answer{};
	const auto answer_size = 4 + ((read_size + 3) & ~std::size_t(3));
	for (;;) {
		if (clock_.Now() >= deadline)
			return failed(I2cResult::timeout);
		memory_result_ = memory_.Transfer(0x17a3f80, true, answer.data(), answer_size);
		if (memory_result_ != MemoryResult::ok)
			return failed(I2cResult::memory_error);
		if (answer[0] == sequence_)
			break;
	}
	if (clock_.Now() >= deadline)
		return failed(I2cResult::timeout);
	if (answer[1] || answer[2])
		return failed(I2cResult::malformed_answer);
	switch (answer[3]) {
	case 0:
		if (read_size)
			std::copy_n(answer.data() + 4, read_size, read);
		return I2cResult::ok;
	case 1: return failed(I2cResult::bus_error);
	case 2: return I2cResult::address_nack;
	case 3: return I2cResult::data_nack;
	case 4: return failed(I2cResult::stuck);
	default: return failed(I2cResult::malformed_answer);
	}
}

} // namespace px4::torne
