// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "dib8096_native_runtime.hpp"

namespace px4::torne {
class NativeFrontend final {
public:
    explicit NativeFrontend(dibnative::Bus& bus);
    ~NativeFrontend();
    NativeFrontend(const NativeFrontend&)=delete;
    NativeFrontend& operator=(const NativeFrontend&)=delete;
    // 模擬バスでは時刻だけを置き換え、実機と同じ状態遷移を検証できる。
    void ConfigureTuning(bool remember_channels,bool reuse_captrim,bool fast_tuning=false);
    bool MemoryHit() const;
    bool MemoryFallback() const;
    bool CaptrimReused() const;
    void SetClock(std::function<dibnative::Time()> now);
    bool Command(std::uint8_t kind,const std::vector<std::uint8_t>& body,std::vector<std::uint8_t>& response);
    const std::string& Error() const noexcept {return error_;}
private:
    bool Execute(std::uint8_t kind,const std::vector<std::uint8_t>& body,std::vector<std::uint8_t>& response);
    struct State;
    std::unique_ptr<State> state_;
    std::string error_;
    bool initialized_=false;
};
}
