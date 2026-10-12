#pragma once
#include "guest_camera.hpp"
#include <algorithm>
#include <limits>

namespace crafterhunter::guest_camera {
// Compare framing only: native projection Z/W depth terms remain engine-owned.
// The managed row-vector VP has the same memory order as our column-vector VP.
inline bool sameFrameCamera(const Plan& p,float aspect,const float* scene) {
    if(!scene) return false;
    for(unsigned i=0;i<16;++i) if(!std::isfinite(scene[i])) return false;
    auto perspective=p.projection;
    if(!projection(p,aspect,perspective.data())) return false;
    for(unsigned row : {0u,1u,3u}) for(unsigned col=0;col<4;++col) {
        double expected=0,magnitude=0;
        for(unsigned k=0;k<4;++k) {
            const double term=double(perspective[k*4+row])*p.view[col*4+k];
            expected+=term;magnitude+=std::abs(term);
        }
        if(!std::isfinite(expected) || !std::isfinite(magnitude)) return false;
        // Float matrix products may use fused operations or separate multiply/add.
        // This bounds rounding error; it is not a motion/depth/latency allowance.
        const double tolerance=8*std::numeric_limits<float>::epsilon()*std::max(1.0,magnitude);
        if(std::abs(double(scene[col*4+row])-expected)>tolerance) return false;
    }
    return true;
}

// CH_Frame selects a new capture after its caller has read the frame camera.
// Hold guest drawing across a mode change until that supplied camera matches
// a plan of the selected mode. Retain only small CPU plans, never GPU resources.
// Same-mode capture advances do not wait for exact camera/time matching: steady
// walking/flying keeps its existing cadence and same-capture attachment binding.
class FrameModeGate {
    std::array<Plan,32> history_{};
    unsigned next_{};
    std::uint64_t generation_{},request_{},epoch_{};
    std::uint32_t mode_{Unknown};
public:
    static constexpr std::uint32_t Unknown=3;
    void reset() { *this={}; }
    std::uint32_t mode() const { return mode_; }
    bool observe(const Plan& selected,float aspect,const float* scene,std::uint64_t now) {
        if(!selected.generation || !selected.request || !selected.sequence
                || !selected.captureIdentity || selected.mode>2 || now>=selected.expires
                || !std::isfinite(aspect) || aspect<=0 || !scene) {
            reset();return false;
        }
        for(unsigned i=0;i<16;++i) if(!std::isfinite(scene[i])) return false;
        if(generation_!=selected.generation || request_!=selected.request || epoch_!=selected.epoch) {
            reset();generation_=selected.generation;request_=selected.request;epoch_=selected.epoch;
        }
        bool remembered=false;
        for(auto& p:history_) if(p.captureIdentity==selected.captureIdentity) {
            p=selected;remembered=true;break;
        }
        if(!remembered) {history_[next_]=selected;next_=(next_+1)%history_.size();}
        if(mode_==selected.mode) return true;
        for(const auto& p:history_) if(p.captureIdentity && p.mode==selected.mode
                && sameFrameCamera(p,aspect,scene)) {
            mode_=selected.mode;return true;
        }
        return false;
    }
};
}
