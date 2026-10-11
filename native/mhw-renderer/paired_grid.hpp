// Draw arithmetic for the paired guest composite, kept off-target and pure.
//
// The preview splats one quad per `stride` x `stride` cell of the captured
// surface; the full-resolution path splats one quad per captured texel. Both
// counts were previously inline arithmetic at the draw call, which is how the
// letterbox scaling in `frame_composite.hpp` ended up inverted and untested for
// as long as it was. Everything the two paths need to agree on lives here.
#ifndef CRAFTERHUNTER_PAIRED_GRID_HPP
#define CRAFTERHUNTER_PAIRED_GRID_HPP

#include <cstdint>

namespace crafterhunter::paired_grid {

// Source columns actually sampled at this stride: a surface whose width is not
// a multiple of the stride still contributes its last partial cell, so rounding
// up is what keeps the right-hand column from being dropped.
[[nodiscard]] constexpr std::uint32_t columns(std::uint32_t width,
                                             std::uint32_t stride) noexcept {
    return stride == 0 ? 0 : (width + stride - 1) / stride;
}

// Cells sampled at this stride.
[[nodiscard]] constexpr std::uint32_t cells(std::uint32_t width,
                                            std::uint32_t height,
                                            std::uint32_t stride) noexcept {
    return columns(width, stride) * columns(height, stride);
}

// Six vertices per cell: two triangles covering the reprojected cell footprint.
[[nodiscard]] constexpr std::uint32_t splatVertices(std::uint32_t width,
                                                    std::uint32_t height,
                                                    std::uint32_t stride) noexcept {
    return cells(width, height, stride) * 6u;
}

// Historical point budget, retained as the coverage regression baseline. A point
// covers only one HOST pixel, regardless of a captured texel's projected size.
[[nodiscard]] constexpr std::uint32_t pointVertices(std::uint32_t width,
                                                    std::uint32_t height) noexcept {
    return width * height;
}

}  // namespace crafterhunter::paired_grid

#endif  // CRAFTERHUNTER_PAIRED_GRID_HPP