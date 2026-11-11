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
    // skip_emptyは、信号が無いと判断したチャンネルの再探索を省くか。起動・復帰直後の選局では省かない。
    void ConfigureTuning(bool remember_channels,bool reuse_captrim,bool skip_empty=false);
    bool MemoryHit() const;
    bool MemoryFallback() const;
    bool CaptrimReused() const;
    // 直前の選局で探索した回数と、段階ごとの所要時間 (ナノ秒)。ログ専用。
    struct TuneTiming {
        std::int64_t acquisitions,tuner_ns,agc_ns,search_ns,sync_ns;
        // 探索失敗ごとのr526/r533 (先頭3回分)、失敗回数、信号なしと判断したか、再探索を省いたか、起動・復帰直後か。
        std::int64_t failures;unsigned fail_noise[3],fail_ber[3];bool no_signal,skipped,fresh;
    };
    TuneTiming LastTuneTiming() const;
    // 直前の選局が状態機械の期限切れで探索失敗として返されたか。ログ専用。
    bool TuneTimedOut() const noexcept {return tune_timed_out_;}
    void SetClock(std::function<dibnative::Time()> now);
    bool Command(std::uint8_t kind,const std::vector<std::uint8_t>& body,std::vector<std::uint8_t>& response);
    const std::string& Error() const noexcept {return error_;}
private:
    bool Execute(std::uint8_t kind,const std::vector<std::uint8_t>& body,std::vector<std::uint8_t>& response);
    struct State;
    std::unique_ptr<State> state_;
    std::string error_;
    bool initialized_=false;
    bool tune_timed_out_=false;
};
}
