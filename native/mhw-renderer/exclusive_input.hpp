// Off-target policy for exclusive-input suppression.
//
// This header is deliberately pure: it contains no game headers, no hooking and
// no absolute addresses, so it can be unit tested on the build host. The hook
// itself lives in renderer.cpp; everything it must decide is here.
//
// Why the decision is object-based rather than code-based: the predicate is
// reached through the object's vtable (`call [rax+0x30]`, `rax = [obj]`), and
// that vtable is shared. 72 objects carried the cMove vtable 0x1431c53e0 in one
// observed scene. Patching the vtable slot, or the shared predicate body, would
// therefore hit every entity in the level. Scoping is only possible by
// comparing the object pointer seen at the call site.
//
// Semantics this encodes, each traceable to a verified observation:
//   * the local cMove predicate is forced to ineligible (-1), so no run starts;
//   * the local cMoveEnd predicate is forced to eligible (0) when it would
//     refuse, so a run that started before exclusive mode can end. The engine
//     consults cMoveEnd, not cMove, while a run is in progress, so both
//     rewrites are required and neither substitutes for the other;
//   * cMoveTurn is never rewritten, so turning is a known, measured gap;
//   * the registered callback propagates -1 unchanged for the move predicate,
//     so the engine takes its own native "not eligible" path instead of a
//     fabricated state;
//   * anything unresolved fails open (forward) rather than acting on memory
//     whose identity has not been proven.
#ifndef CRAFTERHUNTER_EXCLUSIVE_INPUT_HPP
#define CRAFTERHUNTER_EXCLUSIVE_INPUT_HPP

#include <cstdint>

namespace crafterhunter::exclusive_input {

// What the hook observes at the predicate call site.
struct TransitionView {
    const void* object;  // instance pointer (RCX at the call site)
    const void* vtable;  // [object]
    const void* player;  // [object + 0x10]
};

// Instances resolved for the local player, re-resolved on every scene load.
// Addresses are never persisted: they change between sessions.
struct LocalInstances {
    const void* player = nullptr;
    const void* human = nullptr;
    const void* c_move = nullptr;
    const void* c_move_turn = nullptr;
    const void* c_move_end = nullptr;

    // The owner keeps several cMove slots for the same player and uses one at a
    // time. Observed live: three objects whose [obj+0x10] is the local player.
    // All of them belong to the local hunter, so all must be covered; picking a
    // single one is a race. This array holds the additional slots beyond
    // `c_move`, which always holds the first.
    const void* const* c_move_slots = nullptr;
    unsigned c_move_slot_count = 0;

    // Everything needed to act must be present. A partially resolved set is
    // treated as unresolved so a half-finished scan can never filter anything.
    [[nodiscard]] bool resolved() const noexcept {
        return player != nullptr && human != nullptr && c_move != nullptr &&
               c_move_turn != nullptr && c_move_end != nullptr;
    }

    [[nodiscard]] bool ownsCMove(const void* object) const noexcept {
        if (object == nullptr || !resolved()) {
            return false;
        }
        if (object == c_move) {
            return true;
        }
        for (unsigned i = 0; i < c_move_slot_count; ++i) {
            if (c_move_slots[i] == object) {
                return true;
            }
        }
        return false;
    }
};

enum class Action {
    forward,   // run the original predicate untouched
    suppress,  // force the predicate result to ineligible (-1)
    release,   // force the predicate result to eligible (0): end the move
};

// Identity test for the local cMove object.
//
// Requires the vtable AND [obj+0x10] == player. Both are needed: the vtable is
// shared by every cMove in the scene, and [obj+0x10] alone is not proof that
// the object belongs to this hunter.
[[nodiscard]] inline bool is_local_c_move(
    const TransitionView& view,
    const LocalInstances& local
) noexcept {
    if (view.object == nullptr || view.vtable == nullptr ||
        view.player == nullptr) {
        return false;
    }
    if (view.player != local.player) {
        return false;
    }
    return local.ownsCMove(view.object);
}

// The policy. Fails open in every uncertain case.
//
// The two rewritten objects are chosen because the engine's transition tables
// split start from end, not because the two classes are related. Measured on
// build 421810 from the loaded image, for the local hunter's set T = H+0xa00:
//
//   * the per-state table for the idle state (1,0) contains cMove (index 0x93,
//     required predicate result 0), so the move predicate is consulted every
//     frame while at rest and never while a run is in progress;
//   * the per-state table for the running state (1,4) contains cMoveEnd (index
//     0x94, required result 0, destination (1,3)) and cMoveTurn (0x9b) and no
//     cMove at all, so suppressing cMove cannot stop a run that already began.
//
// Suppressing only cMove is therefore correct for starting and structurally
// incapable of stopping. Forcing the end predicate's ineligible result to
// eligible is the other half: it asks the engine for the end transition it
// would have selected had the stick been released, and the engine applies it
// through its own path. At rest the end predicate already returns eligible, so
// the rewrite changes nothing while idle.
[[nodiscard]] inline Action decide(
    const TransitionView& view,
    const LocalInstances& local,
    bool enabled
) noexcept {
    if (!enabled) {
        return Action::forward;
    }
    if (!local.resolved()) {
        return Action::forward;
    }
    if (is_local_c_move(view, local)) {
        // Tested before the end/turn guard so that an aliased resolution
        // (c_move_end == c_move) still suppresses the move rather than
        // releasing it.
        return Action::suppress;
    }
    if (view.object == local.c_move_end && view.player == local.player) {
        return Action::release;
    }
    // Turning is a known, documented gap: it is never rewritten.
    // Another entity, or an object we cannot attribute. Forward.
    return Action::forward;
}

// Enable/disable state. Idempotent: enabling twice is the same as once, so a
// duplicated command cannot stack a second hook.
class Gate {
public:
    void enable() noexcept { enabled_ = true; }
    void disable() noexcept { enabled_ = false; }

    // Drops resolved instances, forcing the next decision to forward. Call on
    // scene load, player replacement and unload.
    void invalidate() noexcept { local_ = LocalInstances{}; }

    void set_local(const LocalInstances& local) noexcept { local_ = local; }

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    [[nodiscard]] const LocalInstances& local() const noexcept { return local_; }

    [[nodiscard]] Action evaluate(const TransitionView& view) const noexcept {
        return decide(view, local_, enabled_);
    }

private:
    bool enabled_ = false;
    LocalInstances local_{};
};

}  // namespace crafterhunter::exclusive_input

#endif  // CRAFTERHUNTER_EXCLUSIVE_INPUT_HPP