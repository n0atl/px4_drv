// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
#include "mb86h47_stream.hpp"
#include "mb86h47_programs.hpp"
#include <algorithm>
#include <array>

namespace px4::torne {
void StreamEngine::Block(std::uint32_t address, std::uint8_t *data, std::size_t size, bool read)
{
	if (result_ != StreamResult::ok) return;
	address_ = address;
	memory_result_ = memory_.Transfer(address, read, data, size);
	if (memory_result_ != MemoryResult::ok) result_ = StreamResult::memory_error;
}
std::uint32_t StreamEngine::Read(std::uint32_t address)
{
	std::array<std::uint8_t,4> data{};
	Block(address,data.data(),data.size(),true);
	return (std::uint32_t(data[0])<<24)|(std::uint32_t(data[1])<<16)|(std::uint32_t(data[2])<<8)|data[3];
}
void StreamEngine::Write(std::uint32_t address,std::uint32_t value)
{
	std::array<std::uint8_t,4> data{static_cast<std::uint8_t>(value>>24),static_cast<std::uint8_t>(value>>16),static_cast<std::uint8_t>(value>>8),static_cast<std::uint8_t>(value)};
	Block(address,data.data(),data.size(),false);
}
void StreamEngine::LoadCore0()
{
	Write(0x16ffffc,0xa0000);
	Write(0x1600000,0xffffffff);
	for (std::size_t first=0;first<programs::core0.size() && result_==StreamResult::ok;first+=16) {
		const auto count=(std::min)(std::size_t(16),programs::core0.size()-first);
		const auto address=0x16a0000+static_cast<std::uint32_t>(first*4);
		std::array<std::uint8_t,64> loaded{},wanted{};
		Block(address,loaded.data(),count*4,true);
		for(std::size_t i=0;i<count;++i)
			for(unsigned byte=0;byte<4;++byte) wanted[i*4+byte]=static_cast<std::uint8_t>(programs::core0[first+i]>>(24-byte*8));
		if(!std::equal(wanted.begin(),wanted.begin()+count*4,loaded.begin())) Block(address,wanted.data(),count*4,false);
	}
}
StreamResult StreamEngine::Prepare()
{
	if(running_ || !bootstrap_.IsInitialized()) return StreamResult::invalid_state;
	prepared_=false;
	result_=StreamResult::ok;
	memory_result_=MemoryResult::ok;
	generation_=bootstrap_.Generation();
	Write(0xd000c4,0);
	Write(0xd00000,0xffffffff);
	Write(0xd00004,0x001f33ff);
	Write(0xd00000,0x30030fff);
	Write(0xd00000,0x00030000);
	Write(0xd00004,0x00003305);
	for(const std::uint32_t address:{0xd01040u,0xd01064u,0xd010a0u,0xd010a4u,0xd010a8u}) Write(address,0);
	Write(0xd01020,0x02000000);
	Write(0xd01004,0);
	Write(0xd01044,0x00bc0000);
	Write(0xd01048,0x47000000);
	Write(0xd0104c,0x00ffffff);
	Write(0xd01050,0);
	Write(0xd01060,0);
	Write(0xd01000,0x06070000);
	Write(0xd01080,0x01000000);
	Write(0xd01088,0);
	Write(0xd0108c,0xff);
	Write(0xd01084,0);
	Write(0xd01090,0x01000000);
	for(const std::uint32_t address:{0xd02028u,0xd0202cu,0xd02030u,0xd02038u}) Write(address,0);
	Write(0xd02000,0x03020300);
	Write(0xd02020,0x00ae4000);
	Write(0xd02024,0x00600000);
	Write(0xd00004,0x00043305);
	LoadCore0();
	Write(0x1600000,0xfffcffff);
	const auto enable=Read(0xfd8010); Write(0xfd8010,enable|0xf0000);
	Write(0xfd8200,1); Write(0xfd8220,1);
	const auto reset=Read(0xfd8014); Write(0xfd8014,reset&~0xf0000u);
	Write(0xfd8240,0); Write(0xfd82a0,0);
	Write(0xfd8200,0); Write(0xfd8204,0xb09f0000); Write(0xfd8200,2);
	Write(0xfd8220,0); Write(0xfd8224,0xb09f2000); Write(0xfd8220,2);
	Write(0xfd8300,1); Write(0xfd8304,0x00008004);
	Write(0xfdbd08,0x00ff6001);
	bool cleared=false;
	for(unsigned attempt=0;attempt<100 && result_==StreamResult::ok;++attempt) {
		if(!(Read(0xfdbd08)&1)) {cleared=true;break;}
	}
	if(result_==StreamResult::ok && !cleared) result_=StreamResult::endpoint_reset_failed;
	Write(0xfdbd08,0x00ff6000);
	Write(0xfdbc14,0x00060030);
	prepared_=result_==StreamResult::ok;
	return result_;
}
StreamResult StreamEngine::Start()
{
	if(!prepared_ || running_ || !bootstrap_.IsInitialized() || generation_!=bootstrap_.Generation() || result_!=StreamResult::ok) return StreamResult::invalid_state;
	Write(0xd01020,0);
	Write(0xd01040,0x03000000);
	Write(0xd010a4,0x13170400);
	Write(0xd00484,0x01000000);
	Write(0xd00488,0x206c0000);
	Write(0xd02010,0x02000000);
	running_=result_==StreamResult::ok;
	return result_;
}
StreamResult StreamEngine::Stop()
{
	if(!generation_ || generation_!=bootstrap_.Generation() || !bootstrap_.IsInitialized()) return StreamResult::invalid_state;
	// 呼出し側で全readを回収してから、core0・入力・writerを停止する。
	result_=StreamResult::ok;
	Write(0x1600000,0xffffffff);
	Write(0xd01040,0);
	Write(0xd01020,0x02000000);
	Write(0xd02010,0x04000000);
	if(result_==StreamResult::ok) running_=prepared_=false;
	return result_;
}
}
