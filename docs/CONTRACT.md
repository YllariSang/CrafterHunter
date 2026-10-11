# Guest camera control contract

This opt-in camera slice uses local atomic files; the existing UDP protocol and
bridge session are unchanged. It is not full gameplay ownership negotiation.

camera.pose in /dev/shm/crafterhunter is exactly 192 little-endian bytes: magic
CHGC u32, version 1 u32, guest generation/request token/sequence u64, F5 mode u32,
flags u32 (=1 means both incoming links OFF), actual camera XYZ f64, camera-relative
view 16 f32 column-major, final uploaded projection 16 f32 column-major. Generation
is WorldCapture's process lifetime. Sequence advances once per sampled level view.
Invalid/nonfinite/unsupported camera or paused guest publishes nothing. No assets.
Native accepts a finite perspective multiplied by a rigid affine camera effect,
verifies the complete matrix reconstruction, moves effects into the centimetre
host view, and keeps host projection Z/W. The uploaded final matrix can change
its W row during walking; it is not required to be a bare perspective matrix.

Controller atomically publishes a single on <token> command after preparing the
existing exclusive-input request. Fabric's guest-control.enabled contains the same
positive token. Native claims the command by same-directory rename before reading.
The native acknowledgement camera.reply is atomically replaced with
<state> <token> <generation> <host epoch> <sequence>, state waiting/active/released.
Fabric shows active only after a matching acknowledgement advances and is locally
fresh. Incoming F7/F9 links cannot be enabled while this mode is requested.

Host selection requires two advancing camera sequences, matching token/generation,
valid existing world anchor, fresh paired world upload, main player identity and
armed movement suppression. Camera receipt timeout is 500ms on the host clock;
Java and Windows clocks are never subtracted. Duplicates cannot refresh the lease,
rollback/malformed frames cannot replace the current plan. Expiry or invalidation
latches released until a new explicit token. F5 never changes the anchor.

Getters consume a copied immutable plan protected by a mutex, never render-thread
world state directly. They forward when inactive/expired and affect only the bound
main camera instance. No queue, spin wait or global task pump. The owned filter
is disabled on release; cached private vtables forward thereafter. Native stop
disables camera hooks before resources are retired, and DLL code remains mapped.

All writers use private temporary files and atomic same-directory replacement.
Stop never deletes the live world/player/camera channels. Fabric restores the
prior incoming-link toggle states when its request flag is removed or F6 releases.
