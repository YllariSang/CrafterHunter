#include "world_upload.hpp"
#include <cassert>
#include <memory>
#include <vector>

using namespace crafterhunter::world;
struct Image {
    Attachment attachment;
    std::uint32_t width, height, pitch;
    std::vector<std::uint8_t> bytes;
};
struct View { std::shared_ptr<Image> image; };
struct Surface {
    std::shared_ptr<Image> texture;
    std::shared_ptr<View> view;
    explicit operator bool() const { return texture && view; }
};
struct Backend {
    unsigned creations{}, views{}, updates{}, step{}, failAt{};
    std::vector<Attachment> updateOrder;
    bool create(Surface& surface, Attachment attachment, const Snapshot& frame) {
        if (++step == failAt) return false;
        ++creations;
        surface.texture = std::make_shared<Image>(Image{
            attachment, frame.width, frame.height, frame.stride,
            attachment == Attachment::Colour ? frame.colour : frame.depth});
        if (++step == failAt) return false;
        ++views;
        surface.view = std::make_shared<View>(View{surface.texture});
        return true;
    }
    void update(Surface& surface, Attachment attachment, const Snapshot& frame) {
        assert(surface && surface.texture->attachment == attachment);
        assert(surface.texture->width == frame.width && surface.texture->height == frame.height);
        assert(surface.texture->pitch == frame.stride);
        ++updates;
        updateOrder.push_back(attachment);
        surface.texture->bytes = attachment == Attachment::Colour ? frame.colour : frame.depth;
    }
    UploadResult upload(UploadPair<Surface>& pair, const Snapshot& frame) {
        return pair.upload(frame,
            [this](auto& surface, auto attachment, const auto& source) {
                return create(surface, attachment, source);
            },
            [this](auto& surface, auto attachment, const auto& source) {
                update(surface, attachment, source);
            });
    }
};

Snapshot snapshot(std::uint32_t width=3, std::uint32_t height=2, std::uint64_t generation=7) {
    Snapshot frame;
    frame.width=width; frame.height=height; frame.stride=width*4;
    frame.generation=generation; frame.identity=1;
    frame.colour.resize(frame.stride*height); frame.depth.resize(frame.stride*height);
    // Asymmetric rows and distinct attachments catch swapped halves/row pitches.
    for (std::size_t i=0; i<frame.colour.size(); ++i) {
        frame.colour[i]=static_cast<std::uint8_t>(i);
        frame.depth[i]=static_cast<std::uint8_t>(255-i);
    }
    return frame;
}

int main() {
    UploadPair<Surface> pair;
    Backend backend;
    auto frame=snapshot();
    assert(backend.upload(pair,frame)==UploadResult::Created);
    assert(backend.creations==2 && backend.views==2 && backend.updates==0);
    auto* colour=pair.colour.texture.get();
    auto* depth=pair.depth.texture.get();
    auto* colourView=pair.colour.view.get();
    auto* depthView=pair.depth.view.get();
    // Regression for the original per-identity allocation path: 120 advancing
    // captures must keep the exact same texture AND view pair.
    for (unsigned i=0; i<120; ++i) {
        ++frame.identity;
        frame.colour[i%frame.colour.size()]^=0x37;
        frame.depth[(i+5)%frame.depth.size()]^=0x91;
        assert(backend.upload(pair,frame)==UploadResult::Updated);
        assert(pair.colour.texture.get()==colour && pair.depth.texture.get()==depth);
        assert(pair.colour.view.get()==colourView && pair.depth.view.get()==depthView);
        assert(pair.colour.view->image==pair.colour.texture && pair.depth.view->image==pair.depth.texture);
        assert(pair.colour.texture->bytes==frame.colour && pair.depth.texture->bytes==frame.depth);
    }
    assert(backend.creations==2 && backend.views==2 && backend.updates==240);
    for (unsigned i=0; i<backend.updateOrder.size(); i+=2) {
        assert(backend.updateOrder[i]==Attachment::Colour);
        assert(backend.updateOrder[i+1]==Attachment::Depth);
    }
    // Source storage may be freed/overwritten once backend update returns.
    auto expected=frame.colour;
    frame.colour.clear(); frame.depth.clear();
    assert(pair.colour.texture->bytes==expected);

    std::weak_ptr<Image> oldColour=pair.colour.texture, oldDepth=pair.depth.texture;
    frame=snapshot(2,3);
    assert(backend.upload(pair,frame)==UploadResult::Created); // same byte size, different shape
    assert(backend.creations==4 && backend.views==4);
    assert(oldColour.expired() && oldDepth.expired());
    assert(pair.width==2 && pair.height==3 && pair.colour.texture->bytes==frame.colour);
    frame.generation=8;
    assert(backend.upload(pair,frame)==UploadResult::Created); // same shape, new producer
    assert(backend.creations==6 && pair.generation==8);
    pair.reset();
    assert(!pair.colour && !pair.depth && !pair.width && !pair.height && !pair.generation);
    assert(backend.upload(pair,frame)==UploadResult::Created); // expiry/disable recovery reallocates

    // Fail at each texture/view creation boundary. No old or half-created pair
    // may remain drawable, and local candidates must be released on failure.
    for (unsigned failure=1; failure<=4; ++failure) {
        frame=snapshot();
        Backend failing;
        assert(failing.upload(pair,frame)==UploadResult::Created);
        oldColour=pair.colour.texture; oldDepth=pair.depth.texture;
        failing.failAt=failing.step+failure;
        frame.width=2; frame.stride=8;
        frame.colour.resize(16); frame.depth.resize(16);
        assert(failing.upload(pair,frame)==UploadResult::Refused);
        assert(!pair.colour && !pair.depth && oldColour.expired() && oldDepth.expired());
        assert(failing.updates==0 && pair.generation==0);
        failing.failAt=0;
        assert(failing.upload(pair,frame)==UploadResult::Created);
    }
    // Validate the full pair before the first update, even on the reuse path.
    for (unsigned malformed=0; malformed<7; ++malformed) {
        frame=snapshot();
        assert(backend.upload(pair,frame)!=UploadResult::Refused);
        if (malformed==0) frame.depth.pop_back();
        if (malformed==1) frame.colour.push_back(0);
        if (malformed==2) frame.stride++;
        if (malformed==3) frame.width=0;
        if (malformed==4) frame.height=MaxDimension+1;
        if (malformed==5) frame.width=MaxDimension+1;
        if (malformed==6) frame.generation=0;
        const auto updates=backend.updates, creations=backend.creations;
        assert(backend.upload(pair,frame)==UploadResult::Refused);
        assert(backend.updates==updates && backend.creations==creations);
        assert(!pair.colour && !pair.depth);
    }
    std::puts("PASS: resident pair reuse, exact rows/halves, resize/restart/reset, all creation failures and malformed-pair refusal (headless, not GPU acceptance)");
}
