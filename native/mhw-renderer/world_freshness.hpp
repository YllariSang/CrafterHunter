#pragma once
#include <array>
#include <cstdint>

namespace crafterhunter::world {
// Host-local receipt liveness, never a subtraction of Java and Windows clocks.
class Freshness {
public:
    static constexpr std::uint64_t TimeoutMillis = 1000;
    enum class Result { Warmup, Advanced, Duplicate, Refused };
    Result observe(std::uint64_t generation, std::uint64_t identity, std::uint64_t now) {
        if (exhausted || !generation || !identity) return Result::Refused;
        if (lastGeneration != generation) {
            for (unsigned i=0; i<count; ++i) if (retired[i]==generation) return Result::Refused;
            if (lastGeneration) {
                if (count==retired.size()) { exhausted=true; ready=false; return Result::Refused; }
                retired[count++]=lastGeneration;
            }
            lastGeneration=generation; lastIdentity=identity; ready=false;
            return Result::Warmup; // An old file at startup is not evidence of a live producer.
        }
        if (identity < lastIdentity) return Result::Refused;
        if (identity == lastIdentity) return Result::Duplicate;
        lastIdentity=identity; advancedAt=now; ready=true;
        return Result::Advanced;
    }
    bool live(std::uint64_t now) const {
        return ready && !exhausted && now>=advancedAt && now-advancedAt<TimeoutMillis;
    }
private:
    std::array<std::uint64_t,4> retired{};
    unsigned count=0;
    std::uint64_t lastGeneration=0,lastIdentity=0,advancedAt=0;
    bool ready=false,exhausted=false;
};
}
