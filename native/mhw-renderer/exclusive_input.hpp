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
//   * the local cMove predicate is forced to ineligible (-1);
//   * cMoveEnd is never touched, so an already active run still stops;
//   * cMoveTurn is never touched, so turning is a known, measured gap;
//   * the registered callback propagates -1 unchanged, so the engine takes its
//     own native "not eligible" path instead of a fabricated state;
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

    // Everything needed to act must be present. A partially resolved set is
    // treated as unresolved so a half-finished scan can never filter anything.
    [[nodiscard]] bool resolved() const noexcept {
        return player != nullptr && human != nullptr && c_move != nullptr &&
               c_move_turn != nullptr && c_move_end != nullptr;
    }
};

enum class Action {
    forward,   // run the original predicate untouched
    suppress,  // force the predicate result to ineligible
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
    return view.object == local.c_move && view.player == local.player;
}

// The policy. Fails open in every uncertain case.
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
    if (view.object == local.c_move_end || view.object == local.c_move_turn) {
        // Release/stop and turn are deliberately preserved.
        return Action::forward;
    }
    if (is_local_c_move(view, local)) {
        return Action::suppress;
    }
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