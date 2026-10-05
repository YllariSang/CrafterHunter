#!/usr/bin/env bash
# Proves the guest's frame layout and the native reader's frame layout are the
# same numbers, rather than agreeing by accident.
#
# The transport is the one place where a Java-written buffer is read by C++: a
# layout change on either side would otherwise show up as a composited frame
# that is flipped, offset, or one pixel behind, which is exactly the kind of bug
# that survives a visual check because it is *almost* right. So both sides state
# the same constants and this compares them.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

# The native side, asked to print its numbers.
cat > "$out/native_sizes.cpp" <<'CPP'
#include <cstdio>
#include "frame_transport.hpp"

int main() {
    using namespace crafterhunter::frame;
    std::printf("magic %08x\n", Magic);
    std::printf("version %u\n", FormatVersion);
    std::printf("format %s\n", FormatName);
    std::printf("bytesPerPixel %u\n", BytesPerPixel);
    std::printf("slotCount %u\n", SlotCount);
    std::printf("headerBytes %zu\n", headerBytes());
    std::printf("slotHeaderBytes %zu\n", slotHeaderBytes());
    std::printf("slotOffset0 %zu\n", slotOffset(0));
    std::printf("slotOffset1 %zu\n", slotOffset(1));
    std::printf("frameBytes1908x1028 %zu\n", frameBytes(1908, 1028));
    std::printf("frameBytes1920x1080 %zu\n", frameBytes(1920, 1080));
    std::printf("pixelsOffset0_1908x1028 %zu\n", pixelsOffset(0, 1908, 1028));
    std::printf("pixelsOffset1_1908x1028 %zu\n", pixelsOffset(1, 1908, 1028));
    std::printf("bufferBytes1908x1028 %zu\n", bufferBytes(1908, 1028));
    std::printf("maxAgeNanos %llu\n", static_cast<unsigned long long>(MaxAgeNanos));
    return 0;
}
CPP

g++ -std=c++20 -Wall -Wextra -Werror \
    -I "$project_root/native/mhw-renderer" \
    "$out/native_sizes.cpp" -o "$out/native_sizes"
"$out/native_sizes" > "$out/native.txt"

# The guest side, asked the same questions.
cat > "$out/GuestSizes.java" <<'JAVA'
import dev.crafterhunter.client.FrameChannel;
import dev.crafterhunter.client.FrameLayout;

public final class GuestSizes {
    public static void main(String[] args) {
        System.out.printf("magic %08x%n", FrameChannel.MAGIC);
        System.out.printf("version %d%n", FrameChannel.FORMAT_VERSION);
        System.out.printf("format %s%n", FrameChannel.FORMAT_NAME);
        System.out.printf("bytesPerPixel %d%n", FrameChannel.BYTES_PER_PIXEL);
        System.out.printf("slotCount %d%n", FrameChannel.SLOT_COUNT);
        System.out.printf("headerBytes %d%n", FrameChannel.HEADER_BYTES);
        System.out.printf("maxAgeNanos %d%n", FrameChannel.MAX_AGE_NANOS);
        System.out.printf("slotHeaderBytes %d%n", FrameChannel.SLOT_HEADER_BYTES);
        System.out.printf("slotOffset0 %d%n", FrameChannel.slotOffset(0));
        System.out.printf("slotOffset1 %d%n", FrameChannel.slotOffset(1));
        System.out.printf("frameBytes1908x1028 %d%n", (int) FrameLayout.frameBytes(1908, 1028));
        System.out.printf("frameBytes1920x1080 %d%n", (int) FrameLayout.frameBytes(1920, 1080));
        System.out.printf("pixelsOffset0_1908x1028 %d%n",
            FrameChannel.pixelsOffsetFor(0, 1908, 1028));
        System.out.printf("pixelsOffset1_1908x1028 %d%n",
            FrameChannel.pixelsOffsetFor(1, 1908, 1028));
        System.out.printf("bufferBytes1908x1028 %d%n", FrameChannel.bufferBytes(1908, 1028));
    }
}
JAVA

sources=(
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameLayout.java"
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameRequest.java"
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameCopyState.java"
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameChannel.java"
)
javac --release 25 -Xlint:all -Werror -d "$out" "${sources[@]}" "$out/GuestSizes.java"
java -cp "$out" GuestSizes > "$out/guest.txt"

python3 - "$out/native.txt" "$out/guest.txt" <<'PY'
import sys

def load(path):
    values = {}
    for line in open(path):
        parts = line.split()
        if len(parts) == 2:
            values[parts[0]] = parts[1]
    return values

native = load(sys.argv[1])
guest = load(sys.argv[2])

# Every key either side prints is comparable, and a key only one side prints is
# itself a failure: it means one side has a rule the other cannot express.
mismatched = []
for key in sorted(set(native) | set(guest)):
    if key not in guest:
        mismatched.append(f"{key}: native {native[key]} has no guest counterpart")
    elif key not in native:
        mismatched.append(f"{key}: guest {guest[key]} has no native counterpart")
    elif native[key] != guest[key]:
        mismatched.append(f"{key}: native {native[key]} != guest {guest[key]}")

if mismatched:
    raise SystemExit("frame layout disagreement:\n  " + "\n  ".join(mismatched))
print(f"Frame layout agrees across both languages on {len(native)} constants:")
for key in sorted(native):
    print(f"  {key} = {native[key]}")
PY
