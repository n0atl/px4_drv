#include <cstdlib>
// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
#include "../src/DriverHost_PX4/torne/dib8096_native.hpp"
#include "../src/DriverHost_PX4/torne/frontend_wait.hpp"
#include <fstream>
#include <iomanip>
#include <map>
#include <deque>
#include <iostream>
#include <thread>
bool TestFlag(const char* name){char* value=nullptr;std::size_t length=0;_dupenv_s(&value,&length,name);const bool enabled=value&&std::string(value)=="1";std::free(value);return enabled;}
using namespace px4::torne;
using namespace px4::torne::dibnative;
class FakeBus final:public Bus {
public:
    std::map<std::uint16_t,std::uint16_t> regs{{896,0x01b3},{897,0x8002}};
    std::map<std::uint16_t,std::uint16_t> tuner{{0x1a,0x0a27}};
    std::map<std::uint16_t,std::deque<std::uint16_t>> queues;
    std::ofstream out;
    Time now{1000000000000};
    unsigned count=0;
    bool failIO=false,failWait=false;
    explicit FakeBus(const std::string& path):out(path){}
    template<class C> void Hex(const C& value){for(std::int64_t i=0;i<Len(value);++i)out<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(value[i]);out<<std::dec;}
    void Locked(){
        queues[1284]={1,0,2};regs[568]=0x3ffe;regs[554]=1;regs[570]=(3<<4)|2;
        regs[493]=1;regs[494]=12;regs[487]=1;regs[488]=3;regs[481]=2;regs[482]=3;
        regs[499]=2;regs[500]=2;regs[505]=1;regs[542]=1<<6;regs[543]=100<<6;regs[390]=65535-50117;
    }
    std::tuple<Slice<std::uint8_t>,Error> WriteRead(std::uint8_t address,Slice<std::uint8_t> data,std::int64_t length) override {
        ++count;
        out<<(length?"R ":"W ")<<unsigned(address)<<" ";Hex(data);
        if(failIO&&count==12){out<<(length?" IOFAIL\n":"\nIOFAIL\n");return {{},MakeError("injected bus failure")};}
        if(address!=0x12&&(address!=0xc0||regs[1025]!=0xc000))throw std::runtime_error("invalid gate/address");
        if(length==0){
            if(address==0x12){if(data.size()!=4)throw std::runtime_error("demod write shape");regs[(data[0]<<8)|data[1]]=(data[2]<<8)|data[3];}
            else{if(data.size()!=3)throw std::runtime_error("tuner write shape");tuner[data[0]]=(data[1]<<8)|data[2];}
            out<<"\n";return {{},nullptr};
        }
        if(length!=2||data.size()!=(address==0x12?2:1))throw std::runtime_error("read shape");
        std::uint16_t value;
        if(address==0x12){auto reg=std::uint16_t((data[0]<<8)|data[1]);value=regs[reg];auto& queue=queues[reg];if(!queue.empty()){value=queue.front();if(queue.size()>1)queue.pop_front();}}
        else value=tuner[data[0]];
        Slice<std::uint8_t> read{std::uint8_t(value>>8),std::uint8_t(value)};out<<" ";Hex(read);out<<"\n";return {read,nullptr};
    }
    Error Wait(std::int64_t duration) override{out<<"S "<<duration<<"\n";if(failWait)return MakeError("injected wait failure");now=now.Add(duration);return nullptr;}
};
int RunFixture(int argc,char** argv){
    try{
        if(argc==1){
            using Clock=std::chrono::steady_clock;
            HANDLE cancel=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!cancel)throw std::runtime_error("cancel event");
            auto begin=Clock::now();auto waited=WaitFrontendDelay(cancel,begin+std::chrono::seconds(1),10000000);
            if(waited||Clock::now()-begin<std::chrono::milliseconds(10))throw std::runtime_error("hardware wait duration");
            if(WaitFrontendDelay(cancel,Clock::now()-std::chrono::milliseconds(1),1000000)!=ERROR_TIMEOUT)throw std::runtime_error("expired wait");
            std::thread stopper([cancel](){Sleep(5);SetEvent(cancel);});
            waited=WaitFrontendDelay(cancel,Clock::now()+std::chrono::seconds(1),500000000);stopper.join();
            if(waited!=ERROR_OPERATION_ABORTED)throw std::runtime_error("cancelled wait");CloseHandle(cancel);
            if(Add<std::uint16_t>(65535,1)!=0||Sub<std::uint32_t>(0,1)!=0xffffffffu||Mul<std::int32_t>(INT32_MAX,2)!=-2||Shl<std::uint32_t>(1,32)!=0||Shr<std::int32_t>(-1,40)!=-1)throw std::runtime_error("integer semantics");
            std::array<int,2> array{};bool bounded=false;try{At(array,2);}catch(const std::out_of_range&){bounded=true;}if(!bounded)throw std::runtime_error("array bounds");
            Slice<int> original{1,2};auto shared=original.sub(1,2);shared[0]=3;if(original[1]!=3)throw std::runtime_error("slice ownership");
            {
                FakeBus resumeBus("NUL");NativeFrontend resume(resumeBus);resume.SetClock([&](){return resumeBus.now;});
                std::vector<std::uint8_t> response;
                if(resume.Command(0x15,{},response)||resumeBus.count)throw std::runtime_error("resume before initialize");
                if(!resume.Command(0x10,{},response)||!resume.Command(0x14,{},response))throw std::runtime_error("resume fixture init/sleep");
                const auto before=resumeBus.count;
                if(!resume.Command(0x15,{},response)||!response.empty()||resumeBus.count-before>10)throw std::runtime_error("resume reinitialized or failed");
                if(resume.Command(0x15,{1},response))throw std::runtime_error("resume accepted body");
                resumeBus.count=11;resumeBus.failIO=true;
                if(resume.Command(0x15,{},response))throw std::runtime_error("resume hid I2C failure");
            }
            for(const auto* mode:{"locked","nosignal","timeout","unknown","iofail","waitfail","cband","remembered","capremembered"}){
                if((std::string(mode)=="remembered"||std::string(mode)=="capremembered")&&!TestFlag("TORNE_TEST_CHANNEL_MEMORY"))continue;
                if(std::string(mode)=="capremembered"&&!TestFlag("TORNE_TEST_CAPTRIM_REUSE"))continue;
                char* fixture[]={argv[0],const_cast<char*>(mode),const_cast<char*>("NUL")};if(RunFixture(3,fixture))return 1;
            }
            std::cout<<"PASS: native frontend mock states, failures, integer semantics and bounds. No USB access.\n";return 0;
        }
        if(argc!=3)return 2;std::string mode=argv[1];FakeBus bus(argv[2]);
        if(mode=="locked"||mode=="cband"||(mode=="remembered"||mode=="capremembered"))bus.Locked();if(mode=="unknown")bus.regs[897]=0x7000;if(mode=="capremembered")bus.tuner[0x1a]=0x0a23;
        bus.failIO=mode=="iofail";bus.failWait=mode=="waitfail";
        NativeFrontend frontend(bus);frontend.ConfigureTuning(TestFlag("TORNE_TEST_CHANNEL_MEMORY"),TestFlag("TORNE_TEST_CAPTRIM_REUSE"),TestFlag("TORNE_TEST_FAST_TUNING"));frontend.SetClock([&](){return bus.now;});
        auto run=[&](std::uint8_t kind,std::uint32_t hz=0){
            std::vector<std::uint8_t> body,response;if(kind==0x11)for(unsigned i=0;i<4;++i)body.push_back(std::uint8_t(hz>>(i*8)));if(kind==0x12)body={0};
            bool ok=frontend.Command(kind,body,response);bus.out<<"C "<<unsigned(kind)<<" "<<unsigned(ok)<<" ";bus.Hex(response);bus.out<<"\n";
            if(kind==0x10){bool failure=mode=="unknown"||mode=="iofail"||mode=="waitfail";if(ok==failure)throw std::runtime_error("initialize outcome");}
            else if(kind==0x11){if(ok==(mode=="timeout"))throw std::runtime_error("tune outcome");if(ok&&((mode=="nosignal")!=(response==std::vector<std::uint8_t>{1})))throw std::runtime_error("search outcome");}
            else if(!ok)throw std::runtime_error("status/output/sleep outcome");
            if(kind==0x13){if(response.size()!=24||bool(response[0])!=(mode=="locked"||mode=="cband"||(mode=="remembered"||mode=="capremembered")))throw std::runtime_error("lock outcome");}
        };
        run(0x10);if(mode=="unknown"||mode=="iofail"||mode=="waitfail")return 0;
        if(mode=="nosignal")bus.queues[1284]={1,0,1};
        run(0x11,mode=="cband"?93142857:473142857);run(0x13);run(0x12);run(0x14);
        if((mode=="remembered"||mode=="capremembered")){bus.queues[1284]={0};run(0x11,473142857);run(0x13);run(0x14);}
        if(mode=="locked"){bus.queues[1284]={1,0,2};run(0x11,213142857);run(0x13);run(0x14);}
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
int main(int argc,char** argv){return RunFixture(argc,argv);}
