#include "../src/protocol.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <span>

int main() {
    constexpr std::array<std::uint8_t, 24> expected{
        'C', 'H', 'N', 'T',
        1, 0,
        3, 0,
        2,
        0, 0, 0,
        4, 3, 2, 1,
        0, 0, 0, 0,
        0, 0, 0, 0,
    };
    const auto packet = crafterhunter::make_packet(
        crafterhunter::Kind::heartbeat,
        0x01020304,
        std::span<const std::uint8_t>{}
    );
    assert(packet.size() == expected.size());
    assert(std::equal(packet.begin(), packet.end(), expected.begin(), expected.end()));
}
