#pragma once
#include "world_frame.hpp"
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
// Minecraft uploads perspective * affine camera effects (walking bob/hurt),
// so the final W row need not be (0,0,-1,0). Recover the actual perspective
// and rigid effect instead of discarding effects or copying guest depth terms.
inline bool projectionParts(const reprojection::Matrix& p,
        reprojection::Matrix& perspective,reprojection::Matrix& effect) {
    if(!reprojection::finite(p)) return false;
    std::array<double,3> z{},x{},y{};
    auto dot=[](const auto& a,const auto& b) {
        double value=0;for(unsigned i=0;i<3;++i)value+=a[i]*b[i];return value;
    };
    for(unsigned col=0;col<3;++col) z[col]=-p[col*4+3];
    const double zz=dot(z,z);
    if(std::abs(zz-1)>1e-4) return false;
    double offsetX=0,offsetY=0,depth=0;
    for(unsigned col=0;col<3;++col) {
        offsetX+=p[col*4]*z[col]/zz;
        offsetY+=p[col*4+1]*z[col]/zz;
        depth+=p[col*4+2]*z[col]/zz;
    }
    for(unsigned col=0;col<3;++col) {
        x[col]=p[col*4]-offsetX*z[col];
        y[col]=p[col*4+1]-offsetY*z[col];
    }
    const double fx=std::sqrt(dot(x,x)),fy=std::sqrt(dot(y,y));
    if(!(fx>1e-8 && fy>1e-8)) return false;
    for(unsigned col=0;col<3;++col) {x[col]/=fx;y[col]/=fy;}
    const double determinant=x[0]*(y[1]*z[2]-y[2]*z[1])
        -x[1]*(y[0]*z[2]-y[2]*z[0])+x[2]*(y[0]*z[1]-y[1]*z[0]);
    if(std::abs(dot(x,y))>1e-4 || std::abs(determinant-1)>1e-4) return false;
    reprojection::Matrix e{},b{};
    for(unsigned col=0;col<3;++col) {
        e[col*4]=x[col];e[col*4+1]=y[col];e[col*4+2]=z[col];
    }
    e[14]=-p[15];e[12]=(p[12]-offsetX*e[14])/fx;
    e[13]=(p[13]-offsetY*e[14])/fy;e[15]=1;
    b[0]=fx;b[5]=fy;b[8]=offsetX;b[9]=offsetY;
    b[10]=depth;b[11]=-1;b[14]=p[14]-depth*e[14];
    reprojection::Matrix inverse{};
    if(!reprojection::finite(e) || !reprojection::inverse(b,inverse)) return false;
    // Refuse other projective shapes: every final term must round-trip.
    for(unsigned col=0;col<4;++col) for(unsigned row=0;row<4;++row) {
        double actual=0;for(unsigned k=0;k<4;++k)actual+=b[k*4+row]*e[col*4+k];
        if(std::abs(actual-p[col*4+row])>1e-5*std::max(1.0,std::abs(p[col*4+row]))) return false;
    }
    perspective=b;effect=e;return true;
}
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
    reprojection::Matrix eye{},perspective{},effect{};
    if(!reprojection::eyeToWorld(s.view,s.position,eye)
            || !projectionParts(s.projection,perspective,effect)) return false;
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
    std::uint64_t captureIdentity{}; // render source; sequence still identifies the ownership feed
};
inline bool makePlan(const Sample& s,const alignment::Anchor& anchor,std::uint64_t epoch,Plan& output) {
    reprojection::Matrix eye{},view{},perspective{},effect{};
    if(!alignment::eyeMatrix(anchor,s.generation,epoch,s.view,s.position,eye)
            || !reprojection::inverse(eye,view)
            || !projectionParts(s.projection,perspective,effect)) return false;
    Plan p;p.mode=s.mode;p.generation=s.generation;p.request=s.request;p.sequence=s.sequence;p.epoch=epoch;
    // The game consumes centimetres; convert view translation, never guest angles.
    for(unsigned i=12;i<15;++i) view[i]*=100;
    // Effects translate in guest blocks, just like the mapped camera. Move them
    // into view space so native projection Z/W and near/far stay untouched.
    for(unsigned i=12;i<15;++i) effect[i]*=100;
    reprojection::Matrix effectiveView{};
    for(unsigned col=0;col<4;++col) for(unsigned row=0;row<4;++row)
        for(unsigned k=0;k<4;++k)effectiveView[col*4+row]+=effect[k*4+row]*view[col*4+k];
    for(unsigned i=0;i<16;++i) {
        p.view[i]=static_cast<float>(effectiveView[i]);p.projection[i]=static_cast<float>(perspective[i]);
        if(!std::isfinite(p.view[i]) || !std::isfinite(p.projection[i])) return false;
    }
    output=p;return true;
}
// The moving body and its captured cape/items must see the camera from their
// own issue-time render. The independent newer camera feed remains authoritative
// for ownership/lease liveness, never substitutes a different rendering time.
inline bool makePairedPlan(const world::Snapshot& captured,const Sample& control,
        const alignment::Anchor& anchor,std::uint64_t epoch,Plan& output) {
    if(!captured.identity || !captured.playerKnown || captured.cameraMode>2
            || captured.generation!=control.generation || !control.request
            || !control.sequence || control.flags!=1) return false;
    Sample render=control;
    render.mode=captured.cameraMode;
    render.position=captured.cameraPosition;
    for(unsigned i=0;i<16;++i) {
        render.view[i]=captured.viewRotation[i];
        render.projection[i]=captured.projection[i];
    }
    Plan candidate;
    if(!makePlan(render,anchor,epoch,candidate)) return false;
    candidate.captureIdentity=captured.identity;
    output=candidate;return true;
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
