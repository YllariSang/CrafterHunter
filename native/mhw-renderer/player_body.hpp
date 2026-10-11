#pragma once
#include <cstdint>

namespace crafterhunter::player {
// The private guest depth is eligible only for the exact composition it orders.
// It never replaces/writes MHW's scene depth.
struct GuestDepthStamp {
    std::uint64_t frame{}, generation{}, identity{}, epoch{};
    std::uintptr_t target{}, camera{};
    bool matches(const GuestDepthStamp& other) const {
        return frame && generation && identity && epoch && target && camera
            && frame==other.frame && generation==other.generation && identity==other.identity
            && epoch==other.epoch && target==other.target && camera==other.camera;
    }
};
enum class BodyDepth { Hidden, Private, SharedGuest };
inline BodyDepth bodyDepth(bool pairedRequested, bool bodyExcluded, bool currentGuestDepth) {
    if (!pairedRequested) return BodyDepth::Private;
    // Legacy/unfiltered frames already carry a body. Do not draw another.
    if (!bodyExcluded || !currentGuestDepth) return BodyDepth::Hidden;
    return BodyDepth::SharedGuest;
}
}
