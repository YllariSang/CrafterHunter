# Composition stage timing

2026-10-12: CPU stage instrumentation is built/tested and matching guest/native
artifacts are installed. Managed code is preserved; all three cmp-match tested
builds. Scoped backup: `native/mhw-renderer/build/rollback.stage-timing.mknkcojy/`;
native installer additionally saved `rollback.UtS5zI/`. Both games were verified
closed first. Control starts OFF; body separation/full/player composition and zero
cadence floors remain enabled, and live channels are preserved. Live measurements
await restart and one calibration. This is the next step toward
reducing user-reported slight occlusion delay, not an accepted latency fix.
World.frame stays v5; body/attachments, actual F5 camera, fixed anchor, all
freshness/depth checks and permanent GPU failure handling are unchanged.

The earlier external observer matched 706 publications with native upload logs.
Its 21.11ms median first-seen interval had 21.48ms combined polling uncertainty.
That cannot isolate GPU readback, CPU publication or screen delay. Keep each
clock domain separate in the new measurements too.

## Guest report

Existing `.minecraft/crafterhunter/out/world-capture.txt` now includes fixed-size
process-lifetime aggregates with generation and last successfully published
capture identity. Each stage reports count/min/mean/max to 0.001ms:

| Field | Measured span in Java nanoTime |
| --- | --- |
| issueToCallback | Issue-time stamp to last requested copy callback |
| callbackToService | Last requested callback to entry into publication service |
| bufferCopy | Allocation, mapping and copying both readback buffers to CPU arrays |
| metadata | Metadata construction and colour/depth hashes |
| channelPublish | Serialization and atomic world.frame publication call |
| issueToPublish | Issue-time stamp to successful publication-call return |

Only a successful complete paired channel publication records a sample. Failed
or incomplete publications do not contribute; a duplicate callback cannot move
the original completion timestamp. Callback order can be either colour/depth;
unrequested attachments do not contribute. Diagnostics can spend extra time
writing disk artifacts between stages; the total includes intervening work and
is not required to equal the sum of the listed stages. Streaming has no new
archives or report files: aggregates join its existing summary.

Callback arrival is a completion signal from the existing backend, not a GPU
timestamp or proof of valid pixels. Issue-to-callback also includes CPU setup and
copy submission. Total ends before the following summary write and subsequent
display work. No spin wait, task pumping, buffer reuse change or queue is added.

## Host report

Existing renderer.log gains at most one set of four rows per second while world
upload is enabled. `clock=host-steady interval` identifies durations taken only
within native steady_clock; GetTickCount64 paces reporting, not stage subtraction.

| stage | Measured CPU work |
| --- | --- |
| read | Opening, reading and validating the snapshot, including path fallback |
| duplicate-read | Subset of read: valid unchanged identities refused for upload |
| upload-submit | Private texture creation or ordered resident texture updates |
| service | Entire world-upload service, including early returns and existing logs |

Each row reports count/min/mean/max in milliseconds. `latest-generation` and
`latest-identity` describe the last retained snapshot; they do not bind every
sample in that reporting interval. Upload-submit includes attempted/refused
uploads and D3D call CPU time, not asynchronous GPU completion. Read includes
failed reads; duplicate-read omits warmup/refusal cases. Reporting resets the
bounded aggregate interval; disable/unload resets it too. Reporting overhead
itself is outside service timing. Its effect on performance is unmeasured.

## Collection and next decision

After matching installation, load both offline scenes and calibrate once in first
person. Reactivate control using the existing controller; no F5 re-anchor. Keep
both games visible and Minecraft focused. Collect while walking/look and F5:

```sh
python3 tools/collect-composition-timing.py --seconds 30
```

This collector reads reports/headers/acknowledgements and a fresh log tail. It
changes no flags, requests, settings or control. Output is under ignored project
captures/composition-timing/runtime-*/ with the source collector and raw reports.
Do not run it as a headless test. `--output` accepts a new directory only under
project captures/ or /tmp/; duration is bounded to 1–120 seconds.

Native upload counts and guest timing identity must advance. Files are read
independently, so their simultaneous association is observational; actual body,
pixels and camera binding remains the existing atomic v5 snapshot contract.
AFK rendering limits remain unchanged: idle and moving cases are not a controlled
performance comparison. Save original screenshots and fresh generation/epoch
evidence separately for visible behavior. Neither report measures screen latency
or Present fps. Use the measured expensive stage to choose one bounded change;
do not infer that upload, readback or shader drawing is the bottleneck from code.

Headless checks: `bash tools/test-stage-timing.sh` exercises injected timestamps,
callback/service separation, duplicate callback refusal, incomplete captures,
backward/duplicate samples, duplicate native read cost, early-return service cost
and report pacing/reset. Mutants overwrite the first callback or fold service
delay into copy delay and must fail. Existing capture/transport/body/F5 tests and
both production builds remain required.

## First live result and selected optimization

Ignored evidence: `captures/composition-timing/runtime-20261011-213008Z/baseline/`.
Generation 41941336025126 and calibration epoch 1522 stay fixed. The 30s collector
observes 297 guest summaries and 120 native report rows, ACTIVE throughout, with
all F5 modes selected by the user. One partial guest summary read recovers.
Across 750 guest advances, approximate interval means are 21.30ms to callback,
0.76ms callback-to-service, 1.29ms buffer copy, 9.03ms metadata, 5.75ms publication
and 38.21ms issue-to-publication-return. Derived guest means have rounding error.
Weighted native means: read 5.08ms, duplicate-read 5.48ms, upload-submit 0.50ms,
total service 7.04ms. This is mixed-view CPU profiling, not controlled A/B, screen
latency, GPU completion or proof of a sole limiter.

The selected guest change omits full diagnostic depth/colour scans and hashes
that streaming prepared but discarded. Explicit non-streaming archives still
record all statistics, provenance and requested attachment hashes. Paired v5
pixels, camera and body payload remain unchanged; no readback policy or depth
guard is altered. `python3 tools/test-capture-metadata.py` executes this production
preparation block and rejects the original unconditional-work mutant. Live speed
or latency improvement remains pending matching guest installation/restart.

User reports jumpy occlusion and hunter overlap in all views while ACTIVE. The
optimization does not identify or claim to fix that issue. Next collect fresh
actual GPU camera/depth evidence alongside the post-install timing trace.
