#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace crafterhunter::world {
constexpr std::uint32_t Magic = 0x50574843, Version = 1, HeaderBytes = 256, MaxDimension = 4096;
struct Snapshot {
    std::uint32_t width{}, height{}, stride{};
    std::uint64_t generation{}, identity{}, issueNanos{};
    float nearPlane{}, farPlane{};
    std::array<float, 16> projection{};
    std::array<float, 5> pose{}; // x,y,z,pitch,yaw in Minecraft units/degrees
    std::vector<std::uint8_t> colour, depth;
};
inline std::uint32_t u32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | std::uint32_t(p[1]) << 8 | std::uint32_t(p[2]) << 16 | std::uint32_t(p[3]) << 24;
}
inline std::uint64_t u64(const std::uint8_t* p) { return u32(p) | std::uint64_t(u32(p + 4)) << 32; }
inline float f32(const std::uint8_t* p) { auto bits = u32(p); float value; std::memcpy(&value, &bits, 4); return value; }
// Opening once holds one inode across an atomic replacement: header and pixels
// cannot come from different publications. A refusal never replaces output.
inline bool read(const char* path, Snapshot& output) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    const auto size = file.tellg();
    if (size < HeaderBytes || size > std::streamoff(HeaderBytes + std::uint64_t(MaxDimension) * MaxDimension * 8)) return false;
    file.seekg(0);
    std::array<std::uint8_t, HeaderBytes> h{};
    if (!file.read(reinterpret_cast<char*>(h.data()), h.size())) return false;
    if (u32(h.data()) != Magic || u32(h.data()+4) != Version || u32(h.data()+8) != HeaderBytes
            || u32(h.data()+12) != 1 || u32(h.data()+28) != 0) return false;
    Snapshot candidate;
    candidate.width = u32(h.data()+16); candidate.height = u32(h.data()+20); candidate.stride = u32(h.data()+24);
    if (!candidate.width || !candidate.height || candidate.width > MaxDimension || candidate.height > MaxDimension
            || candidate.stride != candidate.width * 4) return false;
    const std::uint64_t bytes = std::uint64_t(candidate.stride) * candidate.height;
    if (u64(h.data()+56) != bytes || u64(h.data()+64) != bytes || size != std::streamoff(HeaderBytes + bytes * 2)) return false;
    candidate.generation=u64(h.data()+32); candidate.identity=u64(h.data()+40); candidate.issueNanos=u64(h.data()+48);
    candidate.nearPlane=f32(h.data()+72); candidate.farPlane=f32(h.data()+76);
    if (!candidate.generation || !candidate.identity || !candidate.issueNanos
            || !std::isfinite(candidate.nearPlane) || !std::isfinite(candidate.farPlane)
            || candidate.nearPlane <= 0 || candidate.farPlane <= candidate.nearPlane) return false;
    for (unsigned i=0; i<16; ++i) {
        candidate.projection[i]=f32(h.data()+80+i*4);
        if (!std::isfinite(candidate.projection[i])) return false;
    }
    for (unsigned i=0; i<5; ++i) {
        candidate.pose[i]=f32(h.data()+144+i*4);
        if (!std::isfinite(candidate.pose[i])) return false;
    }
    for (unsigned i=164; i<HeaderBytes; ++i) if (h[i]) return false;
    candidate.colour.resize(bytes); candidate.depth.resize(bytes);
    if (!file.read(reinterpret_cast<char*>(candidate.colour.data()), bytes)
            || !file.read(reinterpret_cast<char*>(candidate.depth.data()), bytes)) return false;
    output = std::move(candidate);
    return true;
}
}
