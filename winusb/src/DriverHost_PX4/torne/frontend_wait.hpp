// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <chrono>
#include <windows.h>

namespace px4::torne {
// Go版のWindows runtimeと同じ高精度waitable timerを使う。通常の
// WaitForSingleObject(timeout)へ短い待機を渡すとtick単位の遅れが積み重なり、
// 校正・探索が余計に遅くなる。レジスター側の待機時間自体は変更しない。
inline DWORD WaitFrontendDelay(HANDLE cancel,std::chrono::steady_clock::time_point deadline,std::int64_t ns){
    using Clock=std::chrono::steady_clock;
    auto check=[&]()->DWORD{
        const auto state=WaitForSingleObject(cancel,0);
        if(state!=WAIT_TIMEOUT)return state==WAIT_FAILED?GetLastError():ERROR_OPERATION_ABORTED;
        return Clock::now()>=deadline?ERROR_TIMEOUT:ERROR_SUCCESS;
    };
    if(auto error=check())return error;
    if(ns<=0)return ERROR_SUCCESS;
    const auto now=Clock::now();
    const auto remaining=std::chrono::duration_cast<std::chrono::nanoseconds>(deadline-now).count();
    if(remaining<=0)return ERROR_TIMEOUT;
    if(ns>remaining)ns=remaining;
    // 古いWindowsで高精度flagが使えない場合も、通常timerで同じ期限を維持する。
    HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002,TIMER_ALL_ACCESS);
    if(!timer)timer=CreateWaitableTimerExW(nullptr,nullptr,0,TIMER_ALL_ACCESS);
    if(!timer)return GetLastError();
    LARGE_INTEGER due{};due.QuadPart=-(ns/100+(ns%100!=0));
    if(!SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE)){const auto error=GetLastError();CloseHandle(timer);return error;}
    const auto timeout_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(deadline-Clock::now()).count();
    DWORD timeout=0;
    if(timeout_ns>0){const auto ms=timeout_ns/1000000+(timeout_ns%1000000!=0);timeout=ms>=MAXDWORD?MAXDWORD-1:static_cast<DWORD>(ms);}
    const HANDLE handles[]{cancel,timer};
    const auto result=WaitForMultipleObjects(2,handles,FALSE,timeout);
    const DWORD error=result==WAIT_OBJECT_0+1?ERROR_SUCCESS:result==WAIT_OBJECT_0?ERROR_OPERATION_ABORTED:result==WAIT_TIMEOUT?ERROR_TIMEOUT:result==WAIT_FAILED?GetLastError():ERROR_INVALID_DATA;
    CancelWaitableTimer(timer);CloseHandle(timer);
    return error?error:check();
}
}
