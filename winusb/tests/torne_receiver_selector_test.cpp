// SPDX-License-Identifier: GPL-2.0-only
#include "../src/DriverHost_PX4/torne/receiver_selector.hpp"
#include <cstdio>

int main(){
    constexpr GUID torne={0x2924f08b,0x94b8,0x5a87,{0x94,0xb9,0x95,0x79,0x85,0x70,0xcd,0x93}};
    constexpr GUID other={0};
    using px4::torne::MatchesReceiverName;
    constexpr auto name=L"Custom receiver [0123456789abcdef]";
    if(!MatchesReceiverName(name,name,other) || MatchesReceiverName(L"other",name,torne))return 1;
    if(!MatchesReceiverName(L"[0123456789abcdef]",name,torne) ||
       !MatchesReceiverName(L"[0123456789ABCDEF]",name,torne))return 2;
    if(MatchesReceiverName(L"[0123456789abcdef]",name,other))return 3;
    for(const auto bad:{L"[1123456789abcdef]",L"[0123456789abcdeg]",L"[0123456789abcde]",L"0123456789abcdef",L"[]"})
        if(MatchesReceiverName(bad,name,torne))return 4;
    if(MatchesReceiverName(L"[0123456789abcdef]",L"receiver[0123456789abcdef]",torne) ||
       MatchesReceiverName(L"[0123456789abcdef]",L"receiver [0123456789abcdef] extra",torne))return 5;
    std::puts("PASS: receiver shorthand, full names, case, malformed IDs and non-torne isolation");
}
