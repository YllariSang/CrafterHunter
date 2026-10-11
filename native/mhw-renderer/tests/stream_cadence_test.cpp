// Regression tests for the host's half of the pacing contract.
//
// The guest parses the same contract in StreamCadence.java and has its own
// tests; these pin the host's copy so the two cannot drift into disagreeing
// about what "0" or a typo means. Off-target: no D3D, no game, no files.
#include "stream_cadence.hpp"

#include <cstdio>

namespace stream = crafterhunter::stream;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        ++failures;
        std::printf("  FAIL: %s\n", what);
    }
}

void testZeroRemovesTheFloor() {
    check(stream::intervalMillis("0") == 0, "zero means look every frame");
    check(stream::intervalMillis("16") == 16, "an explicit interval is honoured");
    check(stream::intervalMillis("250\n") == 250, "a written newline is tolerated");
    check(stream::intervalMillis("  60  ") == 60, "surrounding whitespace is tolerated");
    check(stream::intervalMillis("1000") == stream::MaxIntervalMillis,
          "the bound itself is accepted");
}

void testAnythingElseFallsBack() {
    check(stream::intervalMillis("") == stream::DefaultIntervalMillis, "empty falls back");
    check(stream::intervalMillis("   ") == stream::DefaultIntervalMillis, "blank falls back");
    check(stream::intervalMillis("every frame") == stream::DefaultIntervalMillis,
          "prose falls back");
    check(stream::intervalMillis("-1") == stream::DefaultIntervalMillis,
          "a minus sign is not a digit, so this falls back rather than wrapping");
    check(stream::intervalMillis("+16") == stream::DefaultIntervalMillis,
          "an explicit plus sign falls back");
    check(stream::intervalMillis("16ms") == stream::DefaultIntervalMillis,
          "a suffixed value falls back");
    check(stream::intervalMillis("5000") == stream::DefaultIntervalMillis,
          "beyond the bound falls back");
    check(stream::intervalMillis("99999999999999999999") == stream::DefaultIntervalMillis,
          "an absurd integer falls back instead of overflowing");
}

// The guest and the host must agree on every input, because the two ends of one
// transport read the same contract in different languages.
void testGuestHostAgreement() {
    struct Case { const char* text; std::uint64_t expected; };
    const Case cases[] = {
        {"0", 0}, {"16", 16}, {"250", 250}, {"1000", 1000},
        {"", 250}, {"  ", 250}, {"every frame", 250}, {"-1", 250},
        {"5000", 250}, {"60 ", 60},
    };
    for (const auto& c : cases) {
        // The Java side refuses the same inputs; these literals are the shared
        // examples from StreamCadenceTest and must keep producing these values.
        check(stream::intervalMillis(c.text) == c.expected, c.text);
    }
}

}  // namespace

int main() {
    std::printf("host stream pacing contract\n");
    testZeroRemovesTheFloor();
    testAnythingElseFallsBack();
    testGuestHostAgreement();

    if (failures != 0) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all checks passed\n");
    return 0;
}