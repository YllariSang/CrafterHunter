#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace crafterhunter {

inline constexpr std::array<std::uint8_t, 4> kMagic{'C', 'H', 'N', 'T'};
inline constexpr std::uint16_t kProtocolVersion = 1;
inline constexpr std::size_t kHeaderLength = 24;
inline constexpr std::size_t kMaxPayloadLength = 1200;

enum class Source : std::uint8_t {
    bridge = 1,
    mhw = 2,
    minecraft = 3,
};

enum class Kind : std::uint16_t {
    hello = 1,
    hello_ack = 2,
    heartbeat = 3,
    camera_state = 10,
};

inline void append_u16(std::vector<std::uint8_t>& output, const std::uint16_t value) {
    output.push_back(static_cast<std::uint8_t>(value));
    output.push_back(static_cast<std::uint8_t>(value >> 8));
}
inline void append_u32(std::vector<std::uint8_t>& output, const std::uint32_t value) {
    output.push_back(static_cast<std::uint8_t>(value));
    output.push_back(static_cast<std::uint8_t>(value >> 8));
    output.push_back(static_cast<std::uint8_t>(value >> 16));
    output.push_back(static_cast<std::uint8_t>(value >> 24));
}

inline std::vector<std::uint8_t> make_packet(
    const Kind kind,
    const std::uint32_t sequence,
    const std::span<const std::uint8_t> payload
) {
    if (payload.size() > kMaxPayloadLength) {
        return {};
    }
    std::vector<std::uint8_t> output;
    output.reserve(kHeaderLength + payload.size());
    output.insert(output.end(), kMagic.begin(), kMagic.end());
    append_u16(output, kProtocolVersion);
    append_u16(output, static_cast<std::uint16_t>(kind));
    output.push_back(static_cast<std::uint8_t>(Source::mhw));
    output.insert(output.end(), {0, 0, 0});
    append_u32(output, sequence);
    append_u32(output, static_cast<std::uint32_t>(payload.size()));
    append_u32(output, 0);
    output.insert(output.end(), payload.begin(), payload.end());
    return output;
}

} // namespace crafterhunter
