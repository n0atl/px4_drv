// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "dib8096_native.hpp"
#include "mb86h47_i2c.hpp"
#include <windows.h>

namespace px4::torne {
class NativeFrontendClient final : private dibnative::Bus {
public:
    NativeFrontendClient(MailboxI2c& bus,HANDLE cancel):bus_(bus),cancel_(cancel),frontend_(*this){}
    void ConfigureTuning(bool remember_channels,bool reuse_captrim,bool fast_tuning){frontend_.ConfigureTuning(remember_channels,reuse_captrim,fast_tuning);}
    bool Command(std::uint8_t kind,const std::vector<std::uint8_t>& body,std::vector<std::uint8_t>& response,
        std::chrono::steady_clock::time_point deadline);
    const std::string& Error() const noexcept{return error_;}
private:
    std::tuple<dibnative::Slice<std::uint8_t>,dibnative::Error> WriteRead(std::uint8_t address,
        dibnative::Slice<std::uint8_t> write,std::int64_t read_length) override;
    dibnative::Error Wait(std::int64_t duration) override;
    dibnative::Error CheckState() const;
    MailboxI2c& bus_;
    HANDLE cancel_;
    NativeFrontend frontend_;
    std::chrono::steady_clock::time_point deadline_{};
    std::string error_;
    // コマンド単位の集計のみ保持し、USB要求や待機時間は変更しない。
    std::uint64_t i2c_count_=0, wait_count_=0;
    std::int64_t i2c_us_=0, wait_us_=0, requested_wait_ns_=0;

};
}
