#include "stage_timing.hpp"
#include <cassert>
#include <iostream>

namespace t=crafterhunter::timing;
int main() {
    t::Report report;
    std::int64_t now=1000000;
    const auto clock=[&now] { return now; };
    const auto service=[&](bool duplicate) {
        t::Scope total(report.stages[t::Service],clock);
        const auto readStart=now;
        now+=3000000;
        report.stages[t::Read].record(readStart,now);
        if(duplicate) {
            report.stages[t::DuplicateRead].record(readStart,now);
            return;
        }
        { t::Scope upload(report.stages[t::Upload],clock); now+=7000000; }
        now+=1000000;
    };
    service(false);service(true);
    assert(report.stages[t::Read].count==2 && report.stages[t::Read].meanMillis()==3);
    assert(report.stages[t::DuplicateRead].count==1 && report.stages[t::DuplicateRead].meanMillis()==3);
    assert(report.stages[t::Upload].count==1 && report.stages[t::Upload].meanMillis()==7);
    assert(report.stages[t::Service].count==2 && report.stages[t::Service].meanMillis()==7);
    const auto before=report.stages[t::Read].count;
    report.stages[t::Read].record(5,4);
    assert(report.stages[t::Read].count==before);
    assert(report.due(5000) && !report.due(5000) && !report.due(5999));
    assert(report.due(6000));
    report.clearInterval();
    assert(report.stages[t::Read].count==0 && report.stages[t::Read].meanMillis()==0);
    assert(!report.due(6000));
    std::cout << "PASS: duplicate read cost, early-return service cost and bounded report intervals\n";
}
