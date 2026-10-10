// Regression tests for the exclusive-input suppression policy.
//
// Built and run by tools/test-exclusive-input.sh. Off-target: no game headers,
// no hooking, no live process.
//
// Each test below pins a failure mode that was actually observed, so a
// regression cannot pass silently.
#include "exclusive_input.hpp"

#include <cstdio>

namespace ex = crafterhunter::exclusive_input;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        ++failures;
        std::printf("  FAIL: %s\n", what);
    }
}

// Observed live 2026-10-11 (build 421810). Addresses stand in for the shapes;
// they are never persisted by the real implementation.
constexpr std::uintptr_t kPlayer = 0x69C60080;
constexpr std::uintptr_t kHuman = 0x5E232C80;
constexpr std::uintptr_t kCMove = 0x5E23E140;
constexpr std::uintptr_t kCMoveTurn = 0x5E23E770;
constexpr std::uintptr_t kCMoveEnd = 0x5E23E020;
// A different entity that happens to use the same cMove vtable.
constexpr std::uintptr_t kOtherCMove = 0x5E240000;
constexpr std::uintptr_t kCMoveVtable = 0x1431C53E0;

ex::LocalInstances resolved() {
    return ex::LocalInstances{
        reinterpret_cast<const void*>(kPlayer),
        reinterpret_cast<const void*>(kHuman),
        reinterpret_cast<const void*>(kCMove),
        reinterpret_cast<const void*>(kCMoveTurn),
        reinterpret_cast<const void*>(kCMoveEnd),
    };
}

ex::TransitionView view(std::uintptr_t object, std::uintptr_t player) {
    return ex::TransitionView{
        reinterpret_cast<const void*>(object),
        reinterpret_cast<const void*>(kCMoveVtable),
        reinterpret_cast<const void*>(player),
    };
}

// Baseline: with the gate disabled nothing is touched.
void testDisabledForwardsEverything() {
    ex::Gate gate;
    gate.set_local(resolved());
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::forward,
          "disabled gate forwards the local cMove");
}

// The core requirement: the local hunter's cMove is suppressed.
void testLocalCMoveIsSuppressed() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::suppress,
          "enabled gate suppresses the local cMove");
}

// Regression: the cMove vtable is shared by every entity (72 observed), so a
// filter that matched on the vtable instead of the object would silence the
// whole level.
void testOtherEntitiesAreNotAffected() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    check(gate.evaluate(view(kOtherCMove, kPlayer)) == ex::Action::forward,
          "another entity's cMove with the same vtable is forwarded");
}

// Regression: release/stop must survive, or a run started before enabling
// would never end. This is the case the handoff called unresolved.
void testCMoveEndIsNeverSuppressed() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    check(gate.evaluate(view(kCMoveEnd, kPlayer)) == ex::Action::forward,
          "cMoveEnd forwards so an active run still stops");
}

// Turning is a known, deliberately accepted gap; it must not be claimed fixed.
void testCMoveTurnIsNeverSuppressed() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    check(gate.evaluate(view(kCMoveTurn, kPlayer)) == ex::Action::forward,
          "cMoveTurn forwards (documented gap)");
}

// Regression: a partially resolved set must not filter anything, otherwise a
// half-finished heap scan could act on memory it has not attributed.
//
// The interesting case is c_move resolved but c_move_end NOT: object identity
// alone would then match and suppress, even though we cannot yet prove that
// end is a distinct object. Nulling c_move instead would pass trivially
// because the object compare fails on its own, which is why this pins the
// end-unresolved case specifically.
void testUnresolvedFailsOpen() {
    ex::Gate gate;
    ex::LocalInstances partial = resolved();
    partial.c_move_end = nullptr;
    gate.set_local(partial);
    gate.enable();
    check(!partial.resolved(), "a partial instance set is not resolved");
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::forward,
          "unresolved instances fail open");
}

// Regression: if the resolution ever aliases c_move_end onto c_move, the
// explicit end/turn guard is the only thing still forwarding cMoveEnd. This
// pins that guard rather than relying on the object compare, which would
// otherwise treat an aliased end as the move object.
void testAliasedEndStillForwards() {
    ex::Gate gate;
    ex::LocalInstances aliased = resolved();
    aliased.c_move_end = aliased.c_move;
    gate.set_local(aliased);
    gate.enable();
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::forward,
          "an aliased c_move_end is never suppressed");
}

// Regression: after a scene load the old object address may be recycled by
// another allocation. Invalidating must stop filtering rather than guess.
void testInvalidateStopsFiltering() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::suppress,
          "suppressed before invalidation");
    gate.invalidate();
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::forward,
          "stale instances are not acted on after invalidation");
}

// A duplicated enable command must not change behaviour or stack anything.
void testEnableIsIdempotent() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    gate.enable();
    gate.enable();
    check(gate.enabled(), "gate reports enabled");
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::suppress,
          "repeated enable leaves the decision unchanged");
}

// Disable must restore normal behaviour immediately, including while enabled.
void testDisableRestoresForwarding() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::suppress,
          "suppressed while enabled");
    gate.disable();
    check(gate.evaluate(view(kCMove, kPlayer)) == ex::Action::forward,
          "disable immediately restores forwarding");
}

// Identity needs both the object and [obj+0x10] == player. An object pointer
// that matches while the player link does not is not ours.
void testPlayerLinkMustMatch() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    constexpr std::uintptr_t kOtherPlayer = 0x6E470700;
    check(gate.evaluate(view(kCMove, kOtherPlayer)) == ex::Action::forward,
          "matching object with a foreign player link is forwarded");
}

// Null observations must never dereference into a decision.
void testNullViewsForward() {
    ex::Gate gate;
    gate.set_local(resolved());
    gate.enable();
    const ex::TransitionView empty{nullptr, nullptr, nullptr};
    check(gate.evaluate(empty) == ex::Action::forward,
          "a null observation forwards");
}

}  // namespace

int main() {
    std::printf("exclusive input suppression policy\n");
    testDisabledForwardsEverything();
    testLocalCMoveIsSuppressed();
    testOtherEntitiesAreNotAffected();
    testCMoveEndIsNeverSuppressed();
    testCMoveTurnIsNeverSuppressed();
    testUnresolvedFailsOpen();
    testAliasedEndStillForwards();
    testInvalidateStopsFiltering();
    testEnableIsIdempotent();
    testDisableRestoresForwarding();
    testPlayerLinkMustMatch();
    testNullViewsForward();

    if (failures != 0) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all checks passed\n");
    return 0;
}