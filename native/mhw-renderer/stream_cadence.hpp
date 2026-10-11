// Poll pacing for the host's side of the paired transport, and the same refusals
// the guest applies to its own publish floor.
//
// The 250 ms host floor was chosen to match the guest's request pacing, not
// measured as a limit: both sides were paced by the same deliberate decision, so
// neither was ever asked what cadence it could sustain. Raising cadence is
// therefore a negotiation, not a constant - the flag's content is the interval in
// milliseconds and zero means "look on every frame".
//
// Anything unreadable, negative or beyond the bound falls back to the old
// interval. A typo must never turn into the fastest setting; that failure would
// be invisible until the machine was on fire.
#ifndef CRAFTERHUNTER_STREAM_CADENCE_HPP
#define CRAFTERHUNTER_STREAM_CADENCE_HPP

#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

namespace crafterhunter::stream {

inline constexpr std::uint64_t DefaultIntervalMillis = 250;
inline constexpr std::uint64_t MaxIntervalMillis = 1000;

// Same contract as StreamCadence.java on the guest. A flag file that says
// nothing readable means the historical interval, which is the safe direction.
[[nodiscard]] inline std::uint64_t intervalMillis(std::string_view content) noexcept {
    while (!content.empty() && (content.front() == ' ' || content.front() == '\t'
            || content.front() == '\r' || content.front() == '\n')) {
        content.remove_prefix(1);
    }
    while (!content.empty() && (content.back() == ' ' || content.back() == '\t'
            || content.back() == '\r' || content.back() == '\n')) {
        content.remove_suffix(1);
    }
    if (content.empty()) return DefaultIntervalMillis;
    for (const char c : content) {
        if (c < '0' || c > '9') return DefaultIntervalMillis;
    }
    if (content.size() > 7) return DefaultIntervalMillis;  // far beyond any bound below
    const unsigned long long value = std::strtoull(std::string(content).c_str(), nullptr, 10);
    if (value > MaxIntervalMillis) return DefaultIntervalMillis;
    return value;
}

}  // namespace crafterhunter::stream

#endif  // CRAFTERHUNTER_STREAM_CADENCE_HPP