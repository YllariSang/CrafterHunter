#pragma once
#include <array>
#include <chrono>
#include <cstdint>

namespace crafterhunter::timing {
inline std::int64_t nanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct Stats {
    std::uint64_t count{};
    double total{};
    std::int64_t min{},max{};
    void record(std::int64_t begin,std::int64_t end) {
        if(end<begin) return;
        const auto span=end-begin;
        min=count ? (span<min ? span:min):span;
        max=span>max ? span:max;
        ++count;total+=static_cast<double>(span);
    }
    double meanMillis() const { return count ? total/count/1e6:0; }
};
enum Stage { Read, DuplicateRead, Upload, Service, Count };
struct Report {
    std::array<Stats,Count> stages{};
    std::uint64_t nextReport{};
    bool due(std::uint64_t now) {
        if(now<nextReport) return false;
        nextReport=now+1000;
        return true;
    }
    void clearInterval() { stages={}; }
};
// Scope exit includes early returns; one report never allocates a history queue.
template<typename Clock> struct Scope {
    Stats& stats;
    Clock clock;
    std::int64_t begin;
    Scope(Stats& target,Clock source):stats(target),clock(source),begin(clock()) {}
    ~Scope() { stats.record(begin,clock()); }
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
};
}
