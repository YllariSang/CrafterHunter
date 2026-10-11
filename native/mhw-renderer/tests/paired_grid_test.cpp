// Regression tests for the paired composite's draw arithmetic.
//
// Off-target: pure arithmetic, no D3D, no game. Built by
// tools/test-paired-grid.sh.
#include "paired_grid.hpp"
#include "world_freshness.hpp"

#include <cstdio>

namespace grid = crafterhunter::paired_grid;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        ++failures;
        std::printf("  FAIL: %s\n", what);
    }
}

// The surface size that produced every paired capture so far.
void testPreviewStrideMatchesTheRecordedDraw() {
    // The preview logged 366,996 vertices at 949x1028 with stride 4. If this
    // regresses, the draw silently stops covering part of the surface.
    check(grid::splatVertices(949, 1028, 4) == 366996,
          "preview draw count reproduces the recorded 366,996 vertices");
}

// Regression: rounding down would drop the right-hand and bottom rows whenever
// the surface is not a multiple of the stride, which is most real window sizes.
void testPartialCellIsStillSampled() {
    check(grid::columns(949, 4) == 238, "width rounds up to whole cells");
    check(grid::columns(1027, 4) == 257, "height rounds up to whole cells");
    check(grid::cells(10, 10, 4) == 9, "10x10 at stride 4 is nine cells");
    check(grid::cells(1, 1, 4) == 1, "a surface smaller than one cell still draws");
}

// Regression: a zero stride would divide by zero here, and the preview passes
// its stride from a constant buffer. Zero must yield zero, not a crash or a
// full-surface draw.
void testZeroStrideIsNotAColumnCount() {
    check(grid::columns(949, 0) == 0, "zero stride yields zero columns");
    check(grid::cells(949, 1028, 0) == 0, "zero stride yields zero cells");
    check(grid::splatVertices(949, 1028, 0) == 0, "zero stride submits nothing");
}

// The historical point budget equals stride-1 cells. Retain this comparison
// as the old coverage baseline while the current path uses six vertices/cell.
void testFullResolutionEqualsStrideOne() {
    check(grid::pointVertices(949, 1028) == 975572, "949x1028 is 975,572 points");
    for (std::uint32_t w = 1; w <= 64; ++w) {
        for (std::uint32_t h = 1; h <= 64; ++h) {
            if (grid::pointVertices(w, h) != grid::cells(w, h, 1)) {
                check(false, "point count and stride-1 cell count disagree");
                return;
            }
        }
    }
    check(true, "point count matches stride-1 cells at every size up to 64x64");
}

// The live 949x509 source was enlarged onto 1920x1080. Full coverage
// requires a footprint per texel; its six vertices replace the old point budget.
void testFullPathGeometryAgainstThePreview() {
    check(grid::splatVertices(949, 509, 1) == 2898246,
          "measured source submits 2,898,246 footprint vertices");
    check(grid::splatVertices(4096, 4096, 1) == 100663296,
          "maximum supported source fits UINT draw budget");
    check(grid::splatVertices(1920, 1080, 1) == 6 * grid::pointVertices(1920, 1080),
          "full coverage costs six vertices per source texel");
}

// The full-resolution report states how stale the pixels it drew were, so the
// accessor that supplies it has to be honest: zero before anything advances,
// never negative if the host clock steps backwards, and otherwise the host-local
// distance since the producer's last advance - not a cross-clock subtraction.
void testUploadAgeIsHonest() {
    crafterhunter::world::Freshness freshness;
    check(freshness.age(500) == 0, "nothing has advanced yet, so age is zero");
    check(freshness.observe(7, 11, 100) == crafterhunter::world::Freshness::Result::Warmup,
          "the first sighting warms up");
    check(freshness.age(500) == 0, "a warm-up sighting has not advanced the producer");
    check(freshness.observe(7, 12, 300) == crafterhunter::world::Freshness::Result::Advanced,
          "a new identity advances");
    check(freshness.age(500) == 200, "age is the host-local distance since that advance");
    check(freshness.age(299) == 0, "a backwards clock step reports zero, not a huge number");
    check(!freshness.live(1300), "past the timeout the frame is no longer live");
    check(freshness.age(1300) == 1000, "age keeps growing past the timeout");
}

}  // namespace

int main() {
    std::printf("paired composite draw arithmetic\n");
    testPreviewStrideMatchesTheRecordedDraw();
    testPartialCellIsStillSampled();
    testZeroStrideIsNotAColumnCount();
    testFullResolutionEqualsStrideOne();
    testFullPathGeometryAgainstThePreview();
    testUploadAgeIsHonest();

    if (failures != 0) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all checks passed\n");
    return 0;
}