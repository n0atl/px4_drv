// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2026 Till0196; Copyright (C) 2005-9 DiBcom, SA et al.
#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace px4::torne::dibnative {
using Error = std::shared_ptr<std::string>;
inline void ErrorPart(std::ostream& out, const Error& value) { if(value) out << *value; }
template<class T> void ErrorPart(std::ostream& out, const T& value) { out << value; }
template<class... A> Error MakeError(const std::string& text, const A&... args) {
    std::ostringstream out; out << text; ((out << " ", ErrorPart(out,args)),...);
    return std::make_shared<std::string>(out.str());
}

// Goのsliceと同じく、コピー後も要素の参照先を共有する。末尾の切り詰めで
// 校正テーブルや内部状態の寿命が変わらないよう、所有権と範囲を分離する。
template<class T> class Slice {
    std::shared_ptr<std::vector<T>> storage_;
    std::size_t first_=0, size_=0;
public:
    Slice() = default;
    Slice(std::nullptr_t) {}
    Slice(std::initializer_list<T> values):storage_(std::make_shared<std::vector<T>>(values)),size_(values.size()){}
    explicit Slice(std::size_t count):storage_(std::make_shared<std::vector<T>>(count)),size_(count){}
    std::size_t size() const noexcept {return size_;}
    bool operator==(std::nullptr_t) const noexcept {return !storage_;}
    bool operator!=(std::nullptr_t) const noexcept {return bool(storage_);}
    T& operator[](std::int64_t index) const {
        if(index<0 || static_cast<std::uint64_t>(index)>=size_) throw std::out_of_range("DiB8096 slice index");
        return (*storage_)[first_+static_cast<std::size_t>(index)];
    }
    Slice sub(std::int64_t first, std::int64_t last) const {
        if(first<0||last<first||static_cast<std::uint64_t>(last)>size_) throw std::out_of_range("DiB8096 slice range");
        auto value=*this;value.first_+=static_cast<std::size_t>(first);value.size_=static_cast<std::size_t>(last-first);return value;
    }
    void append(T value) {
        auto copy=std::make_shared<std::vector<T>>(); copy->reserve(size_+1);
        for(std::size_t i=0;i<size_;++i)copy->push_back((*this)[i]);
        copy->push_back(std::move(value));storage_=std::move(copy);first_=0;++size_;
    }
};
template<class C> std::int64_t Len(const C& value) {return static_cast<std::int64_t>(value.size());}
template<class C> decltype(auto) At(C& value,std::int64_t index){
    if(index<0||index>=Len(value))throw std::out_of_range("DiB8096 index");return value[static_cast<std::size_t>(index)];
}
template<class C> decltype(auto) At(const C& value,std::int64_t index){
    if(index<0||index>=Len(value))throw std::out_of_range("DiB8096 index");return value[static_cast<std::size_t>(index)];
}
template<class T> T& At(Slice<T> value,std::int64_t index){return value[index];}
template<class T> Slice<T> Subslice(const Slice<T>& value,std::int64_t first,std::int64_t last){return value.sub(first,last);}
template<class T,std::size_t N> Slice<T> Subslice(const std::array<T,N>& value,std::int64_t first,std::int64_t last){
    if(first<0||last<first||last>N)throw std::out_of_range("DiB8096 array range");
    Slice<T> result(static_cast<std::size_t>(last-first));for(auto i=first;i<last;++i)result[i-first]=value[i];return result;
}
template<class T> Slice<T> Append(Slice<T> value,const T& item){value.append(item);return value;}
template<class A,class B> auto GoMax(A a,B b){using R=std::common_type_t<A,B>;return (std::max)(R(a),R(b));}
template<class A,class B> auto GoMin(A a,B b){using R=std::common_type_t<A,B>;return (std::min)(R(a),R(b));}

// Goの固定幅整数の剰余演算と、大きなシフト幅の意味をC++でも保つ。
// 小さい符号なし型の整数昇格によるsigned overflowも避ける。
template<class T> using Unsigned = std::make_unsigned_t<T>;
template<class T,class A,class B> T Add(A a,B b){return static_cast<T>(std::uint64_t(Unsigned<T>(a))+std::uint64_t(Unsigned<T>(b)));}
template<class T,class A,class B> T Sub(A a,B b){return static_cast<T>(std::uint64_t(Unsigned<T>(a))-std::uint64_t(Unsigned<T>(b)));}
template<class T,class A,class B> T Mul(A a,B b){return static_cast<T>(std::uint64_t(Unsigned<T>(a))*std::uint64_t(Unsigned<T>(b)));}
template<class T,class A> T Neg(A a){return Sub<T>(0,a);}
template<class T,class A,class B> T Shl(A a,B b){if(b<0)throw std::out_of_range("negative shift");return b>=sizeof(T)*8?T(0):static_cast<T>(std::uint64_t(Unsigned<T>(a))<<b);}
template<class T,class A,class B> T Shr(A a,B b){if(b<0)throw std::out_of_range("negative shift");if(b>=sizeof(T)*8)return std::is_signed_v<T>&&T(a)<0?T(-1):T(0);return static_cast<T>(T(a)>>b);}
inline std::int64_t BitLength(std::uint8_t value){std::int64_t n=0;while(value){++n;value>>=1;}return n;}
struct Time {
    std::int64_t ns=0;
    Time Add(std::int64_t duration) const {return {ns+duration};}
    bool After(Time other) const {return ns>other.ns;}
    std::int64_t Sub(Time other) const {return ns-other.ns;}
};
inline Time Now(){return {std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()};}

// 復調器はUSBハンドルを所有しない。実機アダプターが操作全体の期限と
// 抜去イベントを確認し、模擬バスは同じ入口で通信と待機を記録する。
struct Bus {
    virtual ~Bus()=default;
    virtual std::tuple<Slice<std::uint8_t>,Error> WriteRead(std::uint8_t address,Slice<std::uint8_t> write,std::int64_t length)=0;
    virtual Error Wait(std::int64_t duration)=0;
    Error Write(std::uint8_t address,Slice<std::uint8_t> write){return std::get<1>(WriteRead(address,write,0));}
};
inline auto I2cWriteRead(Bus* bus,std::uint8_t address,Slice<std::uint8_t> write,std::int64_t length){return bus->WriteRead(address,write,length);}
inline Error I2cWait(Bus* bus,std::int64_t duration){return bus->Wait(duration);}
} // namespace px4::torne::dibnative
