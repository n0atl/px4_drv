// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <guiddef.h>
#include <cstring>
#include <string_view>

namespace px4::torne {
// 通常の完全一致を優先し、標準torne定義だけ[16桁ID]による指定も許可する。
// 受信機の表示名を変更しても、登録名末尾の個体IDで選べる。
inline bool MatchesReceiverName(std::wstring_view requested, std::wstring_view registered,
    const GUID& device_guid) noexcept
{
    if(requested==registered)return true;
    constexpr GUID torne_guid={0x2924f08b,0x94b8,0x5a87,{0x94,0xb9,0x95,0x79,0x85,0x70,0xcd,0x93}};
    if(std::memcmp(&device_guid,&torne_guid,sizeof(GUID)) || requested.size()!=18 ||
        requested.front()!=L'[' || requested.back()!=L']' || registered.size()<19)return false;
    const auto suffix=registered.substr(registered.size()-18);
    if(registered[registered.size()-19]!=L' ' || suffix.front()!=L'[' || suffix.back()!=L']')return false;
    for(std::size_t i=1;i<17;++i){
        auto a=requested[i],b=suffix[i];
        if(a>=L'A'&&a<=L'F')a+=L'a'-L'A';
        if(b>=L'A'&&b<=L'F')b+=L'a'-L'A';
        if(!((a>=L'0'&&a<=L'9')||(a>=L'a'&&a<=L'f')) || a!=b)return false;
    }
    return true;
}
}
