#pragma once
#include "world_frame.hpp"

namespace crafterhunter::world {
enum class Attachment { Colour, Depth };
enum class UploadResult { Refused, Created, Updated };

// One resident pair. The backend owns texture/view creation and the ordered
// full-image updates; this policy is shared by D3D11 and the headless regression.
// Freshness/identity selection belongs to the caller, before upload().
template<class Surface> struct UploadPair {
    Surface colour, depth;
    std::uint32_t width{}, height{};
    std::uint64_t generation{};

    void reset() { *this = {}; }

    template<class Create, class Update>
    UploadResult upload(const Snapshot& snapshot, Create create, Update update) {
        const auto bytes = std::uint64_t(snapshot.stride) * snapshot.height;
        if (!snapshot.generation || !snapshot.width || !snapshot.height
                || snapshot.width > MaxDimension || snapshot.height > MaxDimension
                || snapshot.stride != snapshot.width * 4
                || snapshot.colour.size() != bytes || snapshot.depth.size() != bytes) {
            reset();
            return UploadResult::Refused;
        }
        if (colour && depth && width == snapshot.width && height == snapshot.height
                && generation == snapshot.generation) {
            // Both commands precede publication of this capture's metadata and
            // any draw. Never update just one attachment from a different pair.
            update(colour, Attachment::Colour, snapshot);
            update(depth, Attachment::Depth, snapshot);
            return UploadResult::Updated;
        }
        Surface nextColour, nextDepth;
        if (!create(nextColour, Attachment::Colour, snapshot)
                || !create(nextDepth, Attachment::Depth, snapshot)
                || !nextColour || !nextDepth) {
            reset();
            return UploadResult::Refused;
        }
        colour = std::move(nextColour);
        depth = std::move(nextDepth);
        width = snapshot.width;
        height = snapshot.height;
        generation = snapshot.generation;
        return UploadResult::Created;
    }
};
}
