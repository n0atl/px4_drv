// receiver_base.cpp

#include "receiver_base.hpp"

#include <chrono>
#include <windows.h>

#include "msg.h"

namespace px4 {

namespace {

std::uint32_t mpeg_crc32(const std::uint8_t *data, std::size_t size) noexcept
{
	std::uint32_t crc = 0xffffffff;

	for (std::size_t i = 0; i < size; i++) {
		crc ^= static_cast<std::uint32_t>(data[i]) << 24;
		for (unsigned int bit = 0; bit < 8; bit++)
			crc = (crc & 0x80000000) ? ((crc << 1) ^ 0x04c11db7) : (crc << 1);
	}

	return crc;
}

} // namespace

ReceiverBase::ReceiverBase(unsigned int options)
	: options_(options),
	lock_()
{
	memset(&params_, 0, sizeof(params_));
	stream_buf_.reset(new StreamBuffer());
}

ReceiverBase::~ReceiverBase()
{

}

bool ReceiverBase::GetParameters(px4::command::ParameterSet &param_set) noexcept
{
	std::lock_guard<std::mutex> lock(lock_);

	param_set.system = params_.system;
	param_set.freq = params_.freq;

	for (std::uint32_t i = 0; i < param_set.num; i++) {
		switch (param_set.params[i].type) {
		case px4::command::ParameterType::BANDWIDTH:
			param_set.params[i].value = params_.bandwidth;
			break;

		case px4::command::ParameterType::SYMBOL_RATE:
			param_set.params[i].value = params_.symbol_rate;
			break;

		case px4::command::ParameterType::MODULATION:
			param_set.params[i].value = params_.modulation;
			break;

		case px4::command::ParameterType::TSMF_MODE:
			param_set.params[i].value = params_.tsmf_mode;
			break;

		case px4::command::ParameterType::TSMF_RELATIVE_STREAM:
			param_set.params[i].value = params_.tsmf_relative_stream;
			break;

		case px4::command::ParameterType::TSMF_SLOT_OFFSET:
			param_set.params[i].value = params_.tsmf_slot_offset;
			break;

		case px4::command::ParameterType::STREAM_ID:
			param_set.params[i].value = params_.stream_id;
			break;

		default:
			// unknown parameter type
			return false;
		}
	}

	return true;
}

bool ReceiverBase::SetParameters(const px4::command::ParameterSet &param_set) noexcept
{
	std::lock_guard<std::mutex> lock(lock_);

	params_.system = param_set.system;
	params_.freq = param_set.freq;
	/* TSMF parameters are optional and must not leak from the previous tune. */
	params_.tsmf_mode = 0;
	params_.tsmf_relative_stream = 0;
	params_.tsmf_slot_offset = 0;

	for (std::uint32_t i = 0; i < param_set.num; i++) {
		switch (param_set.params[i].type) {
		case px4::command::ParameterType::BANDWIDTH:
			params_.bandwidth = param_set.params[i].value;
			break;

		case px4::command::ParameterType::SYMBOL_RATE:
			params_.symbol_rate = param_set.params[i].value;
			break;

		case px4::command::ParameterType::MODULATION:
			params_.modulation = param_set.params[i].value;
			break;

		case px4::command::ParameterType::TSMF_MODE:
			params_.tsmf_mode = param_set.params[i].value;
			break;

		case px4::command::ParameterType::TSMF_RELATIVE_STREAM:
			params_.tsmf_relative_stream = param_set.params[i].value;
			break;

		case px4::command::ParameterType::TSMF_SLOT_OFFSET:
			params_.tsmf_slot_offset = param_set.params[i].value;
			break;

		case px4::command::ParameterType::STREAM_ID:
			params_.stream_id = param_set.params[i].value;
			break;

		default:
			// unknown parameter type
			return false;
		}
	}

	stream_buf_->SetTsmfRelativeStream(params_.tsmf_mode != 0,
		static_cast<std::uint8_t>(params_.tsmf_relative_stream),
		static_cast<std::int32_t>(params_.tsmf_slot_offset));

	return true;
}

void ReceiverBase::ClearParameters() noexcept
{
	std::lock_guard<std::mutex> lock(lock_);

	memset(&params_, 0, sizeof(params_));
	stream_buf_->SetTsmfRelativeStream(false, 0, 0);
}

bool ReceiverBase::Tune(std::uint32_t timeout)
{
	int ret = 0;
	std::lock_guard<std::mutex> lock(lock_);

	if ((params_.system == px4::SystemType::ISDB_S) && (options_ & RECEIVER_SAT_SET_STREAM_ID_BEFORE_TUNE)) {
		ret = SetStreamId();
		if (ret)
			return false;
	}

	ret = SetFrequency();
	if (ret)
		return false;

	int i;
	auto begin = std::chrono::steady_clock::now();
	bool locked = false;

	for (i = 0; true; i++) {
		ret = CheckLock(locked);
		if ((!ret && locked) || ret == -ECANCELED)
			break;

		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin);
		if (duration.count() >= timeout) {
			ret = -ETIMEDOUT;
			break;
		}

		Sleep(20);
	}

	if (ret || !locked)
		return false;

	if ((params_.system == px4::SystemType::ISDB_T || params_.system == px4::SystemType::ISDB_C) && (options_ & RECEIVER_WAIT_AFTER_LOCK_TC_T) && (i < 35))
		Sleep((35 - i) * 10);

	if ((params_.system == px4::SystemType::ISDB_S) && !(options_ & RECEIVER_SAT_SET_STREAM_ID_BEFORE_TUNE)) {
		ret = SetStreamId();
		if (ret)
			return false;
	}

	if (options_ & RECEIVER_WAIT_AFTER_LOCK)
		Sleep(200);

	return true;
}

bool ReceiverBase::ReadStats(px4::command::StatSet &stat_set)
{
	std::lock_guard<std::mutex> lock(lock_);

	for (std::uint32_t i = 0; i < stat_set.num; i++) {
		if (ReadStat(stat_set.data[i].type, stat_set.data[i].value))
			return false;
	}

	return true;
}

ReceiverBase::StreamBuffer::StreamBuffer()
	: write_size_(0),
	threshold_size_(0),
	discard_null_packets_on_output_(false),
	discard_tsmf_header_packets_on_isdbc_output_(false),
	tsmf_enabled_(false),
	tsmf_logged_header_(false),
	tsmf_relative_stream_(0),
	tsmf_slot_offset_(0),
	tsmf_slot_(-1),
	tsmf_stream_enabled_{},
	tsmf_slot_stream_{},
	tsmf_input_packets_(0),
	tsmf_selected_packets_(0),
	tsmf_rejected_packets_(0),
	tsmf_header_packets_(0),
	tsmf_tei_packets_(0),
	tsmf_null_packets_(0),
	tsmf_discontinuity_indicators_(0),
	tsmf_cc_gaps_(0),
	tsmf_last_cc_{},
	tsmf_cc_valid_{}
{

}

ReceiverBase::StreamBuffer::~StreamBuffer()
{

}

void ReceiverBase::StreamBuffer::SetThresholdSize(std::size_t size) noexcept
{
	threshold_size_ = size;
}

bool ReceiverBase::StreamBuffer::Alloc(std::size_t size)
{
	return ringbuf_.Alloc(size);
}

void ReceiverBase::StreamBuffer::Start() noexcept
{
	stop_ = false;
	write_size_ = 0;
	{
		std::lock_guard<std::mutex> lock(tsmf_mtx_);
		tsmf_slot_ = -1;
		tsmf_logged_header_ = false;
	}

	ringbuf_.Start();
	return;
}

void ReceiverBase::StreamBuffer::Stop() noexcept
{
	ringbuf_.Stop();

	stop_ = true;
	cond_.notify_all();

	return;
}

void ReceiverBase::StreamBuffer::StopRequest() noexcept
{
	stop_ = true;
	cond_.notify_all();

	return;
}

void ReceiverBase::StreamBuffer::SetDiscardNullPacketsOnOutput(bool enabled) noexcept
{
	std::lock_guard<std::mutex> lock(tsmf_mtx_);
	discard_null_packets_on_output_ = enabled;
}

void ReceiverBase::StreamBuffer::SetDiscardTsmfHeaderPacketsOnIsdbCOutput(bool enabled) noexcept
{
	std::lock_guard<std::mutex> lock(tsmf_mtx_);
	discard_tsmf_header_packets_on_isdbc_output_ = enabled;
}

void ReceiverBase::StreamBuffer::SetTsmfRelativeStream(bool enabled, std::uint8_t stream, std::int32_t slot_offset) noexcept
{
	std::lock_guard<std::mutex> lock(tsmf_mtx_);

	tsmf_enabled_ = enabled;
	tsmf_relative_stream_ = stream;
	tsmf_slot_offset_ = slot_offset;
	tsmf_slot_ = -1;
	tsmf_logged_header_ = false;
	tsmf_stream_enabled_.fill(0);
	tsmf_slot_stream_.fill(0);
	tsmf_input_packets_ = 0;
	tsmf_selected_packets_ = 0;
	tsmf_rejected_packets_ = 0;
	tsmf_header_packets_ = 0;
	tsmf_tei_packets_ = 0;
	tsmf_null_packets_ = 0;
	tsmf_discontinuity_indicators_ = 0;
	tsmf_cc_gaps_ = 0;
	tsmf_last_cc_.fill(0);
	tsmf_cc_valid_.fill(0);
}

bool ReceiverBase::StreamBuffer::Write(const void *buf, std::size_t &size) noexcept
{
	{
		std::lock_guard<std::mutex> lock(tsmf_mtx_);
		if (tsmf_enabled_) {
			if (size != 188) {
				size = 0;
				return true;
			}

			const auto *packet = static_cast<const std::uint8_t *>(buf);
			tsmf_input_packets_++;
			if (!TsmfSelectPacket(packet)) {
				tsmf_rejected_packets_++;
				if ((tsmf_input_packets_ % 100000) == 0) {
					msg_info("TSMF stats: relative=%u offset=%d input=%llu selected=%llu rejected=%llu headers=%llu tei=%llu null=%llu discontinuity=%llu cc_gaps=%llu\n",
						static_cast<unsigned int>(tsmf_relative_stream_),
						tsmf_slot_offset_,
						static_cast<unsigned long long>(tsmf_input_packets_),
						static_cast<unsigned long long>(tsmf_selected_packets_),
						static_cast<unsigned long long>(tsmf_rejected_packets_),
						static_cast<unsigned long long>(tsmf_header_packets_),
						static_cast<unsigned long long>(tsmf_tei_packets_),
						static_cast<unsigned long long>(tsmf_null_packets_),
						static_cast<unsigned long long>(tsmf_discontinuity_indicators_),
						static_cast<unsigned long long>(tsmf_cc_gaps_));
				}
				size = 0;
				return true;
			}

			tsmf_selected_packets_++;
			if (packet[1] & 0x80)
				tsmf_tei_packets_++;
			/* This is an output-side continuity check.  Null packets have no
			 * meaningful continuity, and an adaptation-field discontinuity
			 * indicator explicitly resets the counter for its PID. */
			const std::uint16_t pid =
				(static_cast<std::uint16_t>(packet[1] & 0x1f) << 8) | packet[2];
			const std::uint8_t adaptation_control = packet[3] & 0x30;
			bool discontinuity = false;
			if ((adaptation_control & 0x20) != 0 && packet[4] > 0 && packet[4] <= 183)
				discontinuity = (packet[5] & 0x80) != 0;
			if (pid == 0x1fff) {
				tsmf_null_packets_++;
			} else if (discontinuity) {
				tsmf_discontinuity_indicators_++;
				tsmf_cc_valid_[pid] = 0;
			} else if ((adaptation_control & 0x10) != 0) {
				const std::uint8_t cc = packet[3] & 0x0f;
				if (tsmf_cc_valid_[pid] && cc != ((tsmf_last_cc_[pid] + 1) & 0x0f))
					tsmf_cc_gaps_++;
				tsmf_last_cc_[pid] = cc;
				tsmf_cc_valid_[pid] = 1;
			}

			if ((tsmf_input_packets_ % 100000) == 0) {
				msg_info("TSMF stats: relative=%u offset=%d input=%llu selected=%llu rejected=%llu headers=%llu tei=%llu null=%llu discontinuity=%llu cc_gaps=%llu\n",
					static_cast<unsigned int>(tsmf_relative_stream_),
					tsmf_slot_offset_,
					static_cast<unsigned long long>(tsmf_input_packets_),
					static_cast<unsigned long long>(tsmf_selected_packets_),
					static_cast<unsigned long long>(tsmf_rejected_packets_),
					static_cast<unsigned long long>(tsmf_header_packets_),
					static_cast<unsigned long long>(tsmf_tei_packets_),
					static_cast<unsigned long long>(tsmf_null_packets_),
					static_cast<unsigned long long>(tsmf_discontinuity_indicators_),
					static_cast<unsigned long long>(tsmf_cc_gaps_));
			}
		}

		/* For TSMF, this point is reached only after a packet has survived
		 * the 52-slot selector.  For ordinary TS, it is the direct output
		 * path.  In both cases PID 0x1fff is now safe padding to discard. */
		if ((discard_null_packets_on_output_ ||
			discard_tsmf_header_packets_on_isdbc_output_) && size == 188) {
			const auto *packet = static_cast<const std::uint8_t *>(buf);
			const std::uint16_t pid =
				(static_cast<std::uint16_t>(packet[1] & 0x1f) << 8) | packet[2];
			if (packet[0] == 0x47 &&
				((discard_null_packets_on_output_ && pid == 0x1fff) ||
				 (discard_tsmf_header_packets_on_isdbc_output_ && pid == 0x002f))) {
				size = 0;
				return true;
			}
		}
	}

	bool ret;

	ret = ringbuf_.Write(buf, size);
	write_size_ += size;

	return ret;
}

bool ReceiverBase::StreamBuffer::TsmfParseHeader(const std::uint8_t *packet) noexcept
{
	const std::uint16_t pid = (static_cast<std::uint16_t>(packet[1]) << 8) | packet[2];
	const std::uint16_t frame_sync = (static_cast<std::uint16_t>(packet[4]) << 8) | packet[5];

	if (packet[0] != 0x47 || pid != 0x002f || (packet[3] & 0xf0) != 0x10 ||
		((frame_sync & 0x1fff) != 0x1a86 && (frame_sync & 0x1fff) != 0x0579) ||
		(packet[6] & 0x10) != 0 || (packet[6] & 0x0f) != 0x01 ||
		mpeg_crc32(&packet[4], 184) != 0)
		return false;

	for (std::size_t i = 0; i < tsmf_stream_enabled_.size(); i++)
		tsmf_stream_enabled_[i] = packet[7 + (i / 8)] & (0x80 >> (i % 8));
	for (std::size_t i = 0; i < tsmf_slot_stream_.size(); i++) {
		const std::uint8_t slot = packet[73 + (i / 2)];
		tsmf_slot_stream_[i] = (i & 1) ? (slot & 0x0f) : (slot >> 4);
	}

	tsmf_slot_ = 0;
	tsmf_header_packets_++;
	if (!tsmf_logged_header_) {
		std::uint16_t enabled_mask = 0;
		char slot_map[53];
		for (std::size_t i = 0; i < tsmf_stream_enabled_.size(); i++)
			enabled_mask |= tsmf_stream_enabled_[i] ? (1u << i) : 0;
		for (std::size_t i = 0; i < tsmf_slot_stream_.size(); i++)
			slot_map[i] = "0123456789ABCDEF"[tsmf_slot_stream_[i] & 0x0f];
		slot_map[tsmf_slot_stream_.size()] = '\0';
		msg_info("TSMF header detected: relative stream=%u, slot offset=%d, enabled=0x%04x, slots=%s\n",
			static_cast<unsigned int>(tsmf_relative_stream_), tsmf_slot_offset_, enabled_mask, slot_map);
		tsmf_logged_header_ = true;
	}
	return true;
}

bool ReceiverBase::StreamBuffer::TsmfSelectPacket(const std::uint8_t *packet) noexcept
{
	if (TsmfParseHeader(packet))
		return false;

	if (packet[0] != 0x47 || tsmf_slot_ < 0 || tsmf_slot_ >= 52) {
		tsmf_slot_ = -1;
		return false;
	}

	tsmf_slot_++;
	const std::uint16_t stream = static_cast<std::uint16_t>(tsmf_relative_stream_) + 1;
	int slot = (tsmf_slot_ - 1 + tsmf_slot_offset_) % 52;
	if (slot < 0)
		slot += 52;
	return stream <= 15 && tsmf_stream_enabled_[stream - 1] &&
		tsmf_slot_stream_[slot] == stream;
}

void ReceiverBase::StreamBuffer::NotifyWrite() noexcept
{
	if (write_size_ >= threshold_size_) {
		cond_.notify_all();
		write_size_ -= threshold_size_;
	}
	return;
}

void ReceiverBase::StreamBuffer::HandleRead(std::size_t buf_size, std::function<bool(const void *buf, std::size_t size)> handler)
{
	stop_ = false;

	std::unique_ptr<std::uint8_t[]> buf(new std::uint8_t[buf_size]);
	std::uint8_t *p = buf.get();
	std::unique_lock<std::mutex> lock(mtx_);

	while (!ringbuf_.IsActive() && !stop_)
		cond_.wait(lock);

	while (true) {
		while (!ringbuf_.GetReadableSize() && !stop_)
			cond_.wait(lock);

		if (stop_)
			break;

		std::size_t size = buf_size;

		if (!ringbuf_.Read(p, size))
			break;

		if (!size)
			continue;

		if (!handler(p, size))
			break;
	}

	return;
}

bool ReceiverBase::StreamBuffer::Purge() noexcept
{
	{
		std::lock_guard<std::mutex> lock(tsmf_mtx_);
		tsmf_slot_ = -1;
	}
	return ringbuf_.Purge();
}

} // namespace px4
