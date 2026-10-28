// SPDX-License-Identifier: GPL-2.0-only
#include "frontend_native_client.hpp"
#include "frontend_wait.hpp"
#include "msg.h"

namespace px4::torne {
dibnative::Error NativeFrontendClient::CheckState() const {
    const auto state=WaitForSingleObject(cancel_,0);
    if(state!=WAIT_TIMEOUT)return dibnative::MakeError("native frontend cancelled/wait error",state==WAIT_FAILED?GetLastError():ERROR_OPERATION_ABORTED);
    if(std::chrono::steady_clock::now()>=deadline_)return dibnative::MakeError("native frontend deadline expired");
    return nullptr;
}
std::tuple<dibnative::Slice<std::uint8_t>,dibnative::Error> NativeFrontendClient::WriteRead(
    std::uint8_t address,dibnative::Slice<std::uint8_t> write,std::int64_t length){
    if(auto err=CheckState())return {{},err};
    if(address&1||write.size()>32||length<0||length>32||(!write.size()&&!length))return {{},dibnative::MakeError("invalid native I2C request")};
    std::array<std::uint8_t,32> output{},input{};
    for(std::size_t i=0;i<write.size();++i)output[i]=write[i];
    const auto started=std::chrono::steady_clock::now();
    const auto result=bus_.Transfer(address,output.data(),write.size(),input.data(),static_cast<std::size_t>(length),deadline_);
    ++i2c_count_;
    i2c_us_+=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
    if(result!=I2cResult::ok)return {{},dibnative::MakeError("native I2C failure",static_cast<unsigned>(result),static_cast<unsigned>(bus_.LastMemoryResult()),bus_.LastSystemError())};
    if(auto err=CheckState())return {{},err};
    dibnative::Slice<std::uint8_t> response(static_cast<std::size_t>(length));
    for(std::int64_t i=0;i<length;++i)response[i]=input[static_cast<std::size_t>(i)];
    return {response,nullptr};
}
dibnative::Error NativeFrontendClient::Wait(std::int64_t duration){
    // 直前の書込みは同期I2C完了時に届いている。待機だけを切り詰め、
    // 操作全体の期限を延長せず、抜去時には同じイベントで中断する。
    const auto started=std::chrono::steady_clock::now();
    const auto result=WaitFrontendDelay(cancel_,deadline_,duration);
    ++wait_count_;requested_wait_ns_+=duration;
    wait_us_+=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
    return result?dibnative::MakeError("native frontend wait failed",result):nullptr;
}
bool NativeFrontendClient::Command(std::uint8_t kind,const std::vector<std::uint8_t>& body,
    std::vector<std::uint8_t>& response,std::chrono::steady_clock::time_point deadline){
    deadline_=deadline;error_.clear();response.clear();
    if(auto err=CheckState()){error_=*err;return false;}
    i2c_count_=wait_count_=0;i2c_us_=wait_us_=requested_wait_ns_=0;
    const auto started=std::chrono::steady_clock::now();
    const bool ok=frontend_.Command(kind,body,response);
    if(kind==0x10||kind==0x11) {
        const auto total_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
        msg_info("torne: frontend command=%u ok=%d total_us=%lld i2c_count=%llu i2c_us=%lld wait_count=%llu wait_requested_us=%lld wait_actual_us=%lld memory_hit=%d memory_fallback=%d captrim_reused=%d\n",
            unsigned(kind),ok,static_cast<long long>(total_us),static_cast<unsigned long long>(i2c_count_),
            static_cast<long long>(i2c_us_),static_cast<unsigned long long>(wait_count_),
            static_cast<long long>(requested_wait_ns_/1000),static_cast<long long>(wait_us_),frontend_.MemoryHit(),frontend_.MemoryFallback(),frontend_.CaptrimReused());
    }
    if(!ok){error_=frontend_.Error();return false;}
    if(auto err=CheckState()){error_=*err;response.clear();return false;}
    return true;
}
}
