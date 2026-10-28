// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2026 Till0196; Copyright (C) 2005-9 DiBcom, SA et al.
// C++ adaptation of coax dib8096 at 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405.
#include "dib8096_native.hpp"
#include <cstring>
#include "dib8096_native_generated.inc"

namespace px4::torne::dibnative {
void m_dib8000_modifyWord(T_dib8000* state,std::uint16_t reg,std::uint16_t keep,std::uint16_t set)
{
    // 旧Go helperのproxyBusはModifierを実装しないため、同じread-modify-writeを使う。
    if(state->v_failure->v_err)return;
    const auto value=m_dib8000_readWord(state,reg);
    m_dib8000_writeWord(state,reg,static_cast<std::uint16_t>((value&keep)|set));
}
class GatedBus final: public Bus {
    Bus& parent_;
    T_i2cMaster& master_;
public:
    GatedBus(Bus& parent,T_i2cMaster& master):parent_(parent),master_(master){}
    std::tuple<Slice<std::uint8_t>,Error> WriteRead(std::uint8_t address,Slice<std::uint8_t> write,std::int64_t length) override {
        if(master_.v_gate!=address){
            m_i2cMaster_selectInterface(&master_,v_interfaceTuner);
            m_dib8000_writeWord(master_.v_demod,static_cast<std::uint16_t>(master_.v_baseRegister+1),m_i2cMaster_gateControl(&master_,address>>1,true));
            if(master_.v_demod->v_failure->v_err)return {{},master_.v_demod->v_failure->v_err};
            master_.v_gate=address;
        }
        return parent_.WriteRead(address,write,length);
    }
    Error Wait(std::int64_t duration) override{return parent_.Wait(duration);}
};
}
namespace px4::torne {
using namespace dibnative;
struct NativeFrontend::State {
    T_Demod board{};
    T_dib8000 demod{};
    T_tuner tuner{};
    std::unique_ptr<T_dib0090Config> config;
    GatedBus gated;
    State(Bus& bus):config(v_dib809xDiB0090Config()),gated(bus,demod.v_master){
        board.v_demod=&demod;board.v_tuner=&tuner;
        auto pause=[this,&bus](std::int64_t duration){m_failure_record(&board.v_failure,bus.Wait(duration));};
        demod.v_bus=&bus;demod.v_address=v_Address;demod.v_failure=&board.v_failure;
        demod.v_cfg=v_dib809xDiB8000Config();demod.v_channel=&board.v_channel;
        demod.v_sleep=pause;demod.v_now=Now;demod.v_tuneTimeout=v_defaultTuneTimeout;
        tuner.v_bus=&gated;tuner.v_address=v_tunerAddress;tuner.v_failure=&board.v_failure;
        tuner.v_config=config.get();tuner.v_channel=&board.v_channel;tuner.v_sleep=pause;
        tuner.v_resetLine=[this](std::int64_t on){m_dib8000_setGPIO(&demod,5,0,static_cast<std::uint8_t>(v_boolWord(on==0)));};
        tuner.v_sleepLine=[this](std::int64_t on){m_dib8000_setGPIO(&demod,0,0,static_cast<std::uint8_t>(on));};
        demod.v_agcControl=[this](std::uint8_t restart){m_tuner_dccFreq(&tuner,restart);if(restart==0)m_tuner_setDCServo(&tuner,1);};
        tuner.v_getADCPower=[this](){return m_dib8000_getADCPower(&demod,1);};
        demod.v_tunerSetParams=[this](){return m_Demod_setParamOverride(&board);};
        demod.v_tunerFrequency=[this](){return m_tuner_getFrequency(&tuner);};
    }
};
NativeFrontend::NativeFrontend(Bus& bus):state_(std::make_unique<State>(bus)){}
NativeFrontend::~NativeFrontend()=default;
void NativeFrontend::ConfigureTuning(bool remember_channels,bool reuse_captrim,bool fast_tuning){
    state_->board.v_useMemory=remember_channels;
    state_->tuner.v_reuseCaptrim=reuse_captrim;
    state_->board.v_reuseTemperature=fast_tuning;
    state_->tuner.v_fastDC=fast_tuning;
    state_->demod.v_fastPoll=fast_tuning;
    state_->demod.v_earlyData=fast_tuning;
}
bool NativeFrontend::MemoryHit() const {return state_->board.v_memoryLastUsed;}
bool NativeFrontend::MemoryFallback() const {return state_->board.v_memoryFallback;}
bool NativeFrontend::CaptrimReused() const {return state_->tuner.v_capLastReused;}
void NativeFrontend::SetClock(std::function<Time()> now){state_->demod.v_now=std::move(now);}
bool NativeFrontend::Command(std::uint8_t kind,const std::vector<std::uint8_t>& body,std::vector<std::uint8_t>& response){
    try{return Execute(kind,body,response);}
    catch(const std::exception& e){error_=e.what();response.clear();initialized_=false;return false;}
}
bool NativeFrontend::Execute(std::uint8_t kind,const std::vector<std::uint8_t>& body,std::vector<std::uint8_t>& response){
    response.clear();error_.clear();auto* board=&state_->board;dibnative::Error err;
    if(!initialized_&&kind!=0x10){error_="frontend is not initialized";return false;}
    switch(kind){
    case 0x10:
        if(!body.empty()){error_="invalid initialize command";return false;}
        err=m_Demod_Initialize(board);if(!err)err=m_Demod_SetGPIO(board,1,true);initialized_=!err;break;
    case 0x11:{
        if(body.size()!=4){error_="invalid tune command";return false;}
        const auto hz=std::uint32_t(body[0])|(std::uint32_t(body[1])<<8)|(std::uint32_t(body[2])<<16)|(std::uint32_t(body[3])<<24);
        err=m_Demod_Tune(board,hz);if(!err&&m_Demod_SearchFailed(board))response={1};break;}
    case 0x12:
        if(body.size()!=1||body[0]>v_OutputFIFO){error_="invalid output command";return false;}
        err=m_Demod_SetOutputMode(board,body[0]);break;
    case 0x13:{
        if(!body.empty()){error_="invalid status command";return false;}
        auto result=m_Demod_Status(board);err=std::get<1>(result);if(err)break;
        const auto& status=std::get<0>(result);response.resize(24);
        response[0]=status.v_Locked;response[1]=status.v_HasCNR;response[2]=status.v_HasStrength;
        // Windows x86/x64はlittle endian。元helperと同じIEEE754バイト列を維持する。
        static_assert(sizeof(double)==8);std::memcpy(response.data()+8,&status.v_CNR,8);std::memcpy(response.data()+16,&status.v_StrengthDBm,8);break;}
    case 0x15:
        if(!body.empty()){error_="invalid resume command";return false;}
        err=m_Demod_SetGPIO(board,1,true);break;
    case 0x14:
        if(!body.empty()){error_="invalid sleep command";return false;}
        err=m_Demod_SetGPIO(board,1,false);if(!err)err=m_Demod_Sleep(board);break;
    default:error_="unknown frontend command";return false;
    }
    if(err){error_=*err;response.clear();return false;}return true;
}
}
