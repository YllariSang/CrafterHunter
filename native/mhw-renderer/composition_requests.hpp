#pragma once
#include <cstdint>

namespace crafterhunter::composition {
struct Requests {
    bool full{}, preview{}, player{};
};

// Draw hooks can observe many eligible UI calls in one frame. A disabled or
// refused compositor never trips its success latch, so querying flag files at
// those calls makes "off" pay repeated filesystem costs. Sample at CH_Frame;
// every draw/log in that frame sees the same complete request state. GPU/depth
// refusals still retry as before; this does not latch failed draw attempts.
class FrameRequests {
    std::uint64_t frame_{};
    bool sampled_{};
    Requests requests_{};
public:
    template<class Exists>
    const Requests& sample(std::uint64_t frame, Exists exists) {
        if (!sampled_ || frame != frame_) {
            requests_ = {exists("paired-full.enabled"),
                exists("paired-compose.enabled"), exists("player-compose.enabled")};
            frame_ = frame;
            sampled_ = true;
        }
        return requests_;
    }
    const Requests& value() const { return requests_; }
    void reset() { *this = {}; }
};
}
