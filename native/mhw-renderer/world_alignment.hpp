#pragma once
#include "world_reprojection.hpp"

namespace crafterhunter::alignment {
using reprojection::Matrix;
using Position=std::array<double,3>;
// Explicit calibration, not inferred from whichever F5 camera arrives next.
// Host positions are metres, never raw MHW centimetres. Caller owns lifecycle.
struct Anchor {
    Position guestOrigin{},hostOrigin{};
    double metresPerBlock{1};
    std::uint64_t guestGeneration{},hostEpoch{};
    bool armed{};
    void reset() { *this=Anchor{}; }
};
// Host-local lifecycle. Missing frames are detected on the next observation;
// callers must additionally gate diagnostics on their frame freshness watchdog.
struct Session {
    Anchor anchor{};
    Position previous{};
    std::uint64_t epoch{1},lastMillis{};
    bool seen{};
    void observe(const Position& host,std::uint64_t now) {
        bool usable=true;
        for(double v:host) usable=usable && std::isfinite(v);
        double distance2=0;
        for(unsigned i=0;i<3;++i) distance2+=(host[i]-previous[i])*(host[i]-previous[i]);
        if(!usable || (seen && (now<lastMillis || now-lastMillis>1000 || distance2>625))) {
            anchor.reset();
            if(epoch!=UINT64_MAX) ++epoch;
            else epoch=0; // exhausted: refuse calibration until process restart
        }
        previous=host; lastMillis=now; seen=usable;
    }
    bool calibrate(const Position& guest,const Position& host,std::uint64_t generation,
            std::uint32_t mode) {
        // First-person reference only: rear/front offsets are never origins.
        if(!seen || !epoch || !generation || mode!=0) return false;
        for(unsigned i=0;i<3;++i)
            if(!std::isfinite(guest[i]) || !std::isfinite(host[i])) return false;
        anchor={guest,host,1,generation,epoch,true}; return true;
    }
};
inline bool valid(const Anchor& a,std::uint64_t generation,std::uint64_t epoch) {
    if(!a.armed || !generation || !epoch || a.guestGeneration!=generation
            || a.hostEpoch!=epoch || !std::isfinite(a.metresPerBlock)
            || a.metresPerBlock<=0) return false;
    for(unsigned i=0;i<3;++i)
        if(!std::isfinite(a.guestOrigin[i]) || !std::isfinite(a.hostOrigin[i])) return false;
    return true;
}
// Same XYZ convention as CameraLink/PlayerLink: host = H + s*(guest-G).
// No yaw fit, axis reflection, player-angle reconstruction or automatic re-anchor.
inline bool worldMatrix(const Anchor& a,std::uint64_t generation,std::uint64_t epoch,
        Matrix& output) {
    if(!valid(a,generation,epoch)) return false;
    Matrix m{}; m[15]=1;
    for(unsigned i=0;i<3;++i) {
        m[i*4+i]=a.metresPerBlock;
        m[12+i]=a.hostOrigin[i]-a.metresPerBlock*a.guestOrigin[i];
    }
    if(!reprojection::finite(m)) return false;
    output=m; return true;
}
// Compose the observed render view/position with a fixed session anchor.
// Third-person offsets and camera effects survive; switching F5 never re-anchors.
inline bool eyeMatrix(const Anchor& a,std::uint64_t generation,std::uint64_t epoch,
        const Matrix& view,const Position& camera,Matrix& output) {
    Matrix world{},eye{};
    if(!worldMatrix(a,generation,epoch,world)
            || !reprojection::eyeToWorld(view,camera,eye)) return false;
    Matrix candidate{};
    for(unsigned col=0;col<4;++col)
        for(unsigned row=0;row<4;++row)
            for(unsigned k=0;k<4;++k)
                candidate[col*4+row]+=world[k*4+row]*eye[col*4+k];
    if(!reprojection::finite(candidate)) return false;
    output=candidate; return true;
}
}
