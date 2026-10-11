#pragma once
#include "world_alignment.hpp"
#include <cstdio>
#include <cstring>

namespace crafterhunter::guest_camera {
struct Sample {
    std::uint64_t generation{},request{},sequence{};
    std::uint32_t mode{},flags{};
    alignment::Position position{};
    reprojection::Matrix view{},projection{};
};
inline bool decode(const unsigned char* bytes,std::size_t size,Sample& output) {
    if(size!=192) return false;
    auto u32=[&](unsigned at){std::uint32_t v;std::memcpy(&v,bytes+at,4);return v;};
    auto u64=[&](unsigned at){std::uint64_t v;std::memcpy(&v,bytes+at,8);return v;};
    if(u32(0)!=0x43474843 || u32(4)!=1) return false;
    Sample s; s.generation=u64(8);s.request=u64(16);s.sequence=u64(24);
    s.mode=u32(32);s.flags=u32(36);
    if(!s.generation || !s.request || !s.sequence || s.mode>2 || s.flags!=1) return false;
    for(unsigned i=0;i<3;++i) std::memcpy(&s.position[i],bytes+40+i*8,8);
    for(unsigned i=0;i<16;++i) {
        float v,p; std::memcpy(&v,bytes+64+i*4,4);std::memcpy(&p,bytes+128+i*4,4);
        s.view[i]=v;s.projection[i]=p;
    }
    if(!reprojection::finite(s.view) || !reprojection::finite(s.projection)
            || s.projection[0]<=0 || s.projection[5]<=0
            || std::abs(s.projection[11]+1)>1e-5 || std::abs(s.projection[15])>1e-5) return false;
    reprojection::Matrix eye{};
    if(!reprojection::eyeToWorld(s.view,s.position,eye)) return false;
    output=s;return true;
}
inline bool read(const char* path,Sample& sample) {
    FILE* f=std::fopen(path,"rb");if(!f) return false;
    unsigned char bytes[193]{};auto size=std::fread(bytes,1,sizeof bytes,f);std::fclose(f);
    return decode(bytes,size,sample);
}
struct Plan {
    std::array<float,16> view{},projection{};
    std::uint32_t mode{};
    std::uint64_t generation{},request{},sequence{},epoch{},expires{};
};
inline bool makePlan(const Sample& s,const alignment::Anchor& anchor,std::uint64_t epoch,Plan& output) {
    reprojection::Matrix eye{},view{};
    if(!alignment::eyeMatrix(anchor,s.generation,epoch,s.view,s.position,eye)
            || !reprojection::inverse(eye,view)) return false;
    Plan p;p.mode=s.mode;p.generation=s.generation;p.request=s.request;p.sequence=s.sequence;p.epoch=epoch;
    // The game consumes centimetres; convert view translation, never guest angles.
    for(unsigned i=12;i<15;++i) view[i]*=100;
    for(unsigned i=0;i<16;++i) {
        p.view[i]=static_cast<float>(view[i]);p.projection[i]=static_cast<float>(s.projection[i]);
        if(!std::isfinite(p.view[i]) || !std::isfinite(p.projection[i])) return false;
    }
    output=p;return true;
}
inline bool projection(const Plan& p,float aspect,float* native) {
    if(!std::isfinite(aspect) || aspect<=0) return false;
    const float ratio=(p.projection[5]/p.projection[0])/aspect;
    if(!std::isfinite(ratio) || ratio<=0) return false;
    // Preserve native Z/W and near/far. Actual final guest XY keeps FOV/offsets.
    for(unsigned col=0;col<4;++col) {
        native[col*4]=p.projection[col*4]*ratio;
        native[col*4+1]=p.projection[col*4+1];
    }
    return true;
}
class Lease {
public:
    enum class State { Off, Waiting, Active, Released };
    State state{State::Off};
    std::uint64_t request{},generation{},sequence{},epoch{},advancedAt{},startedAt{};
    void start(std::uint64_t token,std::uint64_t now) {
        // A duplicate command cannot renew an expired token.
        if(!token || token==request) return;
        *this=Lease{};request=token;startedAt=now;state=State::Waiting;
    }
    void release() {if(state!=State::Off) state=State::Released;}
    bool observe(const Sample& sample,std::uint64_t hostEpoch,bool healthy,std::uint64_t now) {
        if(state==State::Off || state==State::Released) return false;
        if(now<startedAt || (state==State::Waiting && now-startedAt>=2500)
                || (state==State::Active && (!healthy || now<advancedAt || now-advancedAt>=500))) {
            release();return false;
        }
        if(!healthy || sample.request!=request || !sample.generation || !sample.sequence) return false;
        if(generation && (sample.generation!=generation || hostEpoch!=epoch)) {release();return false;}
        if(!generation) {
            generation=sample.generation;sequence=sample.sequence;epoch=hostEpoch;
            return false; // old startup file is not a lease
        }
        if(sample.sequence<sequence) return false;
        if(sample.sequence>sequence) {sequence=sample.sequence;advancedAt=now;state=State::Active;}
        return state==State::Active;
    }
    bool live(std::uint64_t now) const {
        return state==State::Active && now>=advancedAt && now-advancedAt<500;
    }
};
}
