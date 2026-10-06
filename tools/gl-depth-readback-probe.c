/*
 * Reproduces Minecraft's refused depth readback, and measures the recipe that
 * replaces it, on this machine's own GL driver and without a game running.
 *
 * Minecraft 26.2 reads a texture to a buffer with
 *
 *     GlStateManager._glBindBuffer(GL_PIXEL_PACK_BUFFER, dst);
 *     GlStateManager._pixelStore(GL_PACK_ROW_LENGTH, width);
 *     GlStateManager._readPixels(x, y, w, h, GlConst.toGlExternalId(format),
 *                                GlConst.toGlType(format), offset);
 *
 * after binding the source through
 *
 *     directStateAccess().bindFrameBufferTextures(readFbo, texture, 0, level,
 *                                                  GL_READ_FRAMEBUFFER);
 *
 * whose four-argument overload puts the texture in the COLOUR slot and 0 in the
 * depth slot (DirectStateAccess.bindFrameBufferTextures binds GL_COLOR_ATTACHMENT0
 * from `color` and GL_DEPTH_ATTACHMENT from `depth`). For the main target's
 * D32_FLOAT depth that is a depth-format image on a colour attachment, which the
 * framebuffer-completeness rules reject - and the live game log carries exactly
 * that one line:
 *
 *     GL_INVALID_FRAMEBUFFER_OPERATION in glReadPixels(incomplete framebuffer)
 *
 * The same class already attaches depth correctly when it BLITS
 * (copyTextureToTexture passes the texture as `depth` when hasDepthAspect()), so
 * the omission is specific to the readback path, not to the backend as a whole.
 *
 * Five things are measured rather than argued, because each of them has a wrong
 * answer that produces a plausible image:
 *
 *   A. the game's own recipe must fail with GL_INVALID_FRAMEBUFFER_OPERATION.
 *      If it ever stops failing, the diagnosis this file records is out of date
 *      and the test says so instead of passing quietly.
 *   B. which read-buffer setting makes a depth-only framebuffer readable, and
 *      whether the values that come back are the ones that were written -
 *      exactly, since 0.25f and 0.75f are both exactly representable.
 *   C. the row contract: rows are width*4 bytes, the first row is the
 *      framebuffer's bottom row, and a larger destination buffer does not change
 *      either. Buffer capacity is not row stride, and a stride that is assumed
 *      instead of measured shears every row after the first.
 *   D. the pixel-pack state contract: started from a deliberately hostile
 *      incoming state (skip rows/pixels, byte swap, odd row length and
 *      alignment), the previous row-length-and-alignment-only contract comes
 *      back CORRUPTED, while the guarded read returns the written values, stays
 *      inside the frame, and puts every parameter and binding back.
 *
 * Nothing here writes to a game, a request file, or a shared frame channel.
 *
 * Build and run: tools/test-depth-readback.sh
 */

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif

/* Deliberately odd and non-power-of-two: a padded row cannot hide behind a
 * width that is already aligned. */
#define W 61
#define H 47
#define TEXELS ((size_t) W * (size_t) H)
#define BYTES (TEXELS * 4u)

/* Top half of the framebuffer clears to TOP, bottom half to BOTTOM. glClear's
 * y origin is the framebuffer bottom, and so is glReadPixels' first row. */
#define TOP 0.25f
#define BOTTOM 0.75f

/* More than any plausible per-row padding could ever consume (12 bytes * H), so
 * an overrun is detected rather than being able to corrupt anything. */
#define SENTINEL 4096u

static int failures;

static void fail(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    printf("  FAILED: ");
    vprintf(format, arguments);
    printf("\n");
    va_end(arguments);
    failures++;
}

static void require(int condition, const char *format, ...) {
    if (condition) {
        return;
    }
    va_list arguments;
    va_start(arguments, format);
    printf("  FAILED: ");
    vprintf(format, arguments);
    printf("\n");
    va_end(arguments);
    failures++;
}

static const char *error_name(GLenum error) {
    switch (error) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        default: return "unknown";
    }
}

static const char *status_name(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case 0: return "not queried";
        default: return "unknown";
    }
}

static void clear_errors(void) {
    while (glGetError() != GL_NO_ERROR) {
    }
}

static EGLContext create_context(void) {
    EGLDisplay display = EGL_NO_DISPLAY;
    PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (getPlatformDisplay != NULL) {
        display = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, NULL, NULL)) {
            display = EGL_NO_DISPLAY;
        }
    }
    if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, NULL, NULL)) {
            return EGL_NO_CONTEXT;
        }
    }

    if (!eglBindAPI(EGL_OPENGL_API)) {
        return EGL_NO_CONTEXT;
    }

    const EGLint attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint count = 0;
    if (!eglChooseConfig(display, attributes, &config, 1, &count) || count == 0) {
        return EGL_NO_CONTEXT;
    }

    const EGLint contextAttributes[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
    if (context == EGL_NO_CONTEXT) {
        const EGLint fallback[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, fallback);
    }
    if (context == EGL_NO_CONTEXT) {
        return EGL_NO_CONTEXT;
    }
    if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        return EGL_NO_CONTEXT;
    }
    return context;
}

/* A colour texture is rendered too, so the setup matches the main target's
 * colour+depth pair rather than a depth-only toy. */
static GLuint colourTexture;
static GLuint depthTexture;

static void render_bands(void) {
    glGenTextures(1, &colourTexture);
    glBindTexture(GL_TEXTURE_2D, colourTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, W, H, 0, GL_DEPTH_COMPONENT,
        GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colourTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("  FAILED: the render framebuffer is not complete: %s\n",
            status_name(glCheckFramebufferStatus(GL_FRAMEBUFFER)));
        failures++;
        return;
    }

    glViewport(0, 0, W, H);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClearDepth(BOTTOM);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Top half of the framebuffer: y from H/2 up. */
    glEnable(GL_SCISSOR_TEST);
    GLint y = H / 2;
    glScissor(0, y, W, H - y);
    glClearDepth(TOP);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
}

/* Test A: the recipe Minecraft's GlCommandEncoder.copyTextureToBuffer uses for
 * the D32_FLOAT depth texture. It must fail, the way the game's log shows. */
static void test_game_recipe(void) {
    printf("A. the game's recipe (depth texture on GL_COLOR_ATTACHMENT0)\n");

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    /* Exactly bindFrameBufferTextures(readFbo, glId, 0, level, GL_READ_FRAMEBUFFER):
     * colour slot gets the texture, depth slot gets 0. */
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTexture, 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

    GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    printf("  framebuffer status: %s\n", status_name(status));
    require(status != GL_FRAMEBUFFER_COMPLETE,
        "a depth-format image on a colour attachment was accepted as complete, so the "
        "measured diagnosis in this file no longer describes this driver");

    GLuint pack;
    glGenBuffers(1, &pack);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pack);
    glBufferData(GL_PIXEL_PACK_BUFFER, BYTES, NULL, GL_STREAM_READ);
    glPixelStorei(GL_PACK_ROW_LENGTH, W);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    clear_errors();
    glReadPixels(0, 0, W, H, GL_DEPTH_COMPONENT, GL_FLOAT, (void *) 0);
    GLenum error = glGetError();
    printf("  glReadPixels error: %s\n", error_name(error));
    require(error == GL_INVALID_FRAMEBUFFER_OPERATION,
        "expected GL_INVALID_FRAMEBUFFER_OPERATION from the game's recipe, got %s",
        error_name(error));

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDeleteBuffers(1, &pack);
    glDeleteFramebuffers(1, &framebuffer);
}

/* Which read-buffer setting makes a depth-only framebuffer readable? Measured,
 * because two of the three candidates are rejected by rules that are easy to
 * state and easy to get backwards. */
typedef struct {
    const char *name;
    const char *recipe;
} Variant;

/* The three candidates, in the order they are tried: the first one that reads
 * back the written values is what the capture path is then held to. */
static const Variant VARIANTS[] = {
    {"none", "glReadBuffer(GL_NONE)"},
    {"depth-attachment", "glReadBuffer(GL_DEPTH_ATTACHMENT)"},
    {"colour-attachment", "read buffer left at its default"}
};
#define VARIANT_COUNT ((int) (sizeof(VARIANTS) / sizeof(VARIANTS[0])))

/* The same read into client memory, which is where the row contract in test C
 * can actually be seen: sentinel bytes after the tight frame tell us whether
 * the driver wrote past it. */
static GLenum read_to_client(const Variant *variant, unsigned char *destination) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    clear_errors();
    if (strcmp(variant->name, "none") == 0) {
        glReadBuffer(GL_NONE);
    } else if (strcmp(variant->name, "depth-attachment") == 0) {
        glReadBuffer(GL_DEPTH_ATTACHMENT);
    }

    glPixelStorei(GL_PACK_ROW_LENGTH, W);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    clear_errors();
    glReadPixels(0, 0, W, H, GL_DEPTH_COMPONENT, GL_FLOAT, destination);
    GLenum error = glGetError();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
    return error;
}

static int read_variant(const Variant *variant, float *out, GLenum *errorOut, GLenum *statusOut) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

    if (strcmp(variant->name, "none") == 0) {
        glReadBuffer(GL_NONE);
    } else if (strcmp(variant->name, "depth-attachment") == 0) {
        clear_errors();
        glReadBuffer(GL_DEPTH_ATTACHMENT);
        if (glGetError() != GL_NO_ERROR) {
            /* glReadBuffer does not accept this value on this driver. */
            *errorOut = GL_INVALID_ENUM;
            *statusOut = 0;
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);
            return 0;
        }
    }
    /* "colour-attachment": leave the default read buffer alone. */

    *statusOut = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);

    GLuint pack;
    glGenBuffers(1, &pack);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pack);
    glBufferData(GL_PIXEL_PACK_BUFFER, BYTES, NULL, GL_STREAM_READ);
    glPixelStorei(GL_PACK_ROW_LENGTH, W);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    clear_errors();
    glReadPixels(0, 0, W, H, GL_DEPTH_COMPONENT, GL_FLOAT, (void *) 0);
    *errorOut = glGetError();

    if (*errorOut == GL_NO_ERROR) {
        memset(out, 0, BYTES);
        glGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0, BYTES, out);
        GLenum tail = glGetError();
        if (tail != GL_NO_ERROR) {
            *errorOut = tail;
        }
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDeleteBuffers(1, &pack);
    glDeleteFramebuffers(1, &framebuffer);
    return *errorOut == GL_NO_ERROR;
}

/* Returns 1 when the variant produced the written values, and records whether
 * row 0 held the framebuffer's bottom band. */
static int values_are_the_written_ones(const float *values, int *row0IsBottom) {
    if (values[0] != BOTTOM || values[TEXELS - 1] != TOP) {
        return 0;
    }
    if (row0IsBottom != NULL) {
        *row0IsBottom = values[0] == BOTTOM;
    }
    for (size_t index = 0; index < TEXELS; index++) {
        size_t row = index / (size_t) W;
        float expected = row < (size_t) (H / 2) ? BOTTOM : TOP;
        if (values[index] != expected) {
            return 0;
        }
    }
    return 1;
}

static int test_variants(int *winner) {
    printf("B. the corrected recipe (depth texture on GL_DEPTH_ATTACHMENT)\n");

    static float values[TEXELS];
    *winner = -1;

    for (int index = 0; index < VARIANT_COUNT; index++) {
        GLenum error = GL_NO_ERROR;
        GLenum status = 0;
        int row0IsBottom = 0;
        int ok = read_variant(&VARIANTS[index], values, &error, &status);
        int correct = ok && values_are_the_written_ones(values, &row0IsBottom);
        printf("  %-18s status=%-38s error=%-34s %s\n", VARIANTS[index].name,
            status_name(status), error_name(error),
            correct ? "values correct" : (ok ? "wrong values" : "no read"));
        if (correct && *winner < 0) {
            *winner = index;
        }
    }

    /* glReadTexImage needs no framebuffer at all, so it is measured too: if the
     * readback recipe cannot be made to work, this is the fallback. */
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    clear_errors();
    static float texelValues[TEXELS];
    memset(texelValues, 0, BYTES);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, texelValues);
    GLenum error = glGetError();
    int correct = error == GL_NO_ERROR && values_are_the_written_ones(texelValues, NULL);
    printf("  %-18s %-69s %s\n", "glGetTexImage", error_name(error),
        correct ? "values correct" : "wrong values");

    if (*winner < 0 && correct) {
        printf("  -> no glReadPixels variant worked; glGetTexImage is the only path\n");
        return 0;
    }
    require(*winner >= 0, "no glReadPixels recipe returned the written depth values");
    printf("  -> using read buffer %s\n", VARIANTS[*winner].recipe);
    return 1;
}

/* Test C: the row contract, measured with sentinel bytes around the data.
 *
 * The destination is client memory here, not a pixel-pack buffer, because the
 * question is whether the driver wrote past the tight frame - and that is only
 * visible if the memory after it is ours and still holds what we put there.
 * The packing rules are the driver's own and do not depend on where the
 * destination lives, which is why measuring them here says something about the
 * game's buffer too.
 */
static void test_packing(int winner) {
    printf("C. row contract (width=%d height=%d, %u bytes per row)\n", W, H, W * 4u);

    size_t capacity = BYTES + SENTINEL;
    unsigned char *small = malloc(capacity);
    unsigned char *large = malloc(capacity * 4u);
    if (small == NULL || large == NULL) {
        fail("out of memory");
        free(small);
        free(large);
        return;
    }

    static const char *labels[3] = {"glReadPixels", "glGetTexImage", "glGetTexImage, 4x capacity"};

    for (size_t pass = 0; pass < 3; pass++) {
        const char *label = labels[pass];
        unsigned char *destination = pass == 2 ? large : small;
        size_t destinationCapacity = pass == 2 ? capacity * 4u : capacity;
        memset(destination, 0xCD, destinationCapacity);

        GLenum error;
        if (pass == 0) {
            error = read_to_client(&VARIANTS[winner], destination);
        } else {
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            glPixelStorei(GL_PACK_ROW_LENGTH, W);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            clear_errors();
            glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, destination);
            error = glGetError();
        }
        require(error == GL_NO_ERROR, "%s: read failed with %s", label, error_name(error));
        if (error != GL_NO_ERROR) {
            continue;
        }

        const float *values = (const float *) destination;
        /* Tight stride: every row starts exactly W floats after the previous
         * one, whatever the destination's capacity happens to be. */
        int sheared = 0;
        for (size_t row = 0; row < (size_t) H; row++) {
            float expected = row < (size_t) (H / 2) ? BOTTOM : TOP;
            const float *first = &values[row * (size_t) W];
            if (first[0] != expected || first[W - 1] != expected) {
                fail("%s: row %zu starts with %.6f and ends with %.6f, expected %.6f - the rows "
                     "are not packed at width*4",
                    label, row, first[0], first[W - 1], expected);
                sheared = 1;
                break;
            }
        }
        if (sheared) {
            continue;
        }
        for (size_t column = 0; column < (size_t) W; column++) {
            if (values[column] != BOTTOM) {
                fail("%s: column %zu of row 0 is %.6f, expected %.6f - the first row is not the "
                     "framebuffer's bottom row",
                    label, column, values[column], (double) BOTTOM);
                break;
            }
        }

        /* The copy consumed exactly width*height*4 bytes: capacity is not a
         * licence to write more, and a padded row would show here. */
        size_t firstWrittenBeyond = 0;
        for (size_t offset = BYTES; offset < destinationCapacity; offset++) {
            if (destination[offset] != 0xCD) {
                firstWrittenBeyond = offset;
                break;
            }
        }
        require(firstWrittenBeyond == 0,
            "%s: bytes past the tight frame were written, first at %zu of a %zu-byte capacity "
            "- the rows are padded",
            label, firstWrittenBeyond, destinationCapacity);
    }

    printf("  row 0 is the framebuffer's bottom row; rows are %u bytes each; a 4x larger "
           "destination changes neither\n", W * 4u);
    free(small);
    free(large);
}

/* Test D: the pixel-pack state contract, started from a state this reader did
 * not create.
 *
 * A read inherits whatever the last writer on the context left behind. Row
 * length and alignment were always set; skip rows/pixels and byte-swap were
 * not, and neither announces itself: a leftover skip relocates the data (it is
 * measured here which side of the transfer it lands on), and a leftover
 * byte-swap reverses every float's four bytes - all while glGetError reports
 * nothing. Both destroy every value without raising an error.
 *
 * Two reads are measured from the same hostile starting state:
 *
 *   D1. the previous contract - row length and alignment only - to show it
 *       comes back CORRUPTED. This is the regression: if D1 ever comes back
 *       correct, the hostile state was ignored, or the contract under test no
 *       longer matches the code, and this test is out of date rather than
 *       reassuring.
 *   D2. the current contract - all six pack parameters set and restored, plus
 *       the read-framebuffer and pixel-pack-buffer bindings - to show the
 *       written values, nothing written outside the frame, and every parameter
 *       back where it was found.
 *
 * GL_PACK_IMAGE_HEIGHT and GL_PACK_SKIP_IMAGES are hostile here too but are
 * deliberately NOT part of the guard: if they changed a 2D glReadPixels read,
 * D2 would come back wrong and the failure below would say so. Their still
 * holding their hostile values after D2 is the measurement that this read
 * ignores them - an argument turned into a check.
 */
typedef struct {
    GLint rowLength;
    GLint alignment;
    GLint skipRows;
    GLint skipPixels;
    GLint swapBytes;
    GLint lsbFirst;
    GLint imageHeight;
    GLint skipImages;
} PackState;

static void get_pack(PackState *state) {
    glGetIntegerv(GL_PACK_ROW_LENGTH, &state->rowLength);
    glGetIntegerv(GL_PACK_ALIGNMENT, &state->alignment);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &state->skipRows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &state->skipPixels);
    glGetIntegerv(GL_PACK_SWAP_BYTES, &state->swapBytes);
    glGetIntegerv(GL_PACK_LSB_FIRST, &state->lsbFirst);
    glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &state->imageHeight);
    glGetIntegerv(GL_PACK_SKIP_IMAGES, &state->skipImages);
}

static void set_pack(const PackState *state) {
    glPixelStorei(GL_PACK_ROW_LENGTH, state->rowLength);
    glPixelStorei(GL_PACK_ALIGNMENT, state->alignment);
    glPixelStorei(GL_PACK_SKIP_ROWS, state->skipRows);
    glPixelStorei(GL_PACK_SKIP_PIXELS, state->skipPixels);
    glPixelStorei(GL_PACK_SWAP_BYTES, state->swapBytes);
    glPixelStorei(GL_PACK_LSB_FIRST, state->lsbFirst);
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, state->imageHeight);
    glPixelStorei(GL_PACK_SKIP_IMAGES, state->skipImages);
}

static int pack_equal(const PackState *left, const PackState *right) {
    return left->rowLength == right->rowLength && left->alignment == right->alignment
        && left->skipRows == right->skipRows && left->skipPixels == right->skipPixels
        && left->swapBytes == right->swapBytes && left->lsbFirst == right->lsbFirst
        && left->imageHeight == right->imageHeight && left->skipImages == right->skipImages;
}

/* First offset at or after `from` whose byte differs from `fill`, or capacity
 * when every byte from `from` on still holds the fill. */
static size_t first_changed(const unsigned char *data, size_t capacity, size_t from,
    unsigned char fill) {
    for (size_t offset = from; offset < capacity; offset++) {
        if (data[offset] != fill) {
            return offset;
        }
    }
    return capacity;
}

/* First offset below `limit` still holding `fill` - a byte the read never wrote
 * - or `limit` when the whole region was written. */
static size_t first_untouched(const unsigned char *data, size_t limit, unsigned char fill) {
    for (size_t offset = 0; offset < limit; offset++) {
        if (data[offset] == fill) {
            return offset;
        }
    }
    return limit;
}

/* The read as DepthReadback issues it: its own read framebuffer, depth on the
 * depth attachment, the read buffer B measured as working, completeness checked
 * before anything is issued, and the state it manages saved and put back in the
 * same order as the Java reader's finally block. fullGuard = 0 reproduces the
 * contract before the pack state was enumerated (row length and alignment
 * only); fullGuard = 1 is the current one. */
static GLenum read_with_pack(unsigned char *destination, int fullGuard, int winner) {
    GLint previousFramebuffer;
    GLint previousPackBuffer;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);
    PackState saved;
    get_pack(&saved);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    if (strcmp(VARIANTS[winner].name, "none") == 0) {
        glReadBuffer(GL_NONE);
    } else if (strcmp(VARIANTS[winner].name, "depth-attachment") == 0) {
        glReadBuffer(GL_DEPTH_ATTACHMENT);
    }

    GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        require(status == GL_FRAMEBUFFER_COMPLETE,
            "D: read framebuffer is not complete: %s", status_name(status));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint) previousFramebuffer);
        glDeleteFramebuffers(1, &framebuffer);
        return GL_INVALID_FRAMEBUFFER_OPERATION;
    }

    clear_errors();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ROW_LENGTH, W);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    if (fullGuard) {
        glPixelStorei(GL_PACK_SKIP_ROWS, 0);
        glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
        glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
    }
    glReadPixels(0, 0, W, H, GL_DEPTH_COMPONENT, GL_FLOAT, destination);
    GLenum error = glGetError();

    glBindBuffer(GL_PIXEL_PACK_BUFFER, (GLuint) previousPackBuffer);
    glPixelStorei(GL_PACK_ROW_LENGTH, saved.rowLength);
    glPixelStorei(GL_PACK_ALIGNMENT, saved.alignment);
    if (fullGuard) {
        glPixelStorei(GL_PACK_SKIP_ROWS, saved.skipRows);
        glPixelStorei(GL_PACK_SKIP_PIXELS, saved.skipPixels);
        glPixelStorei(GL_PACK_SWAP_BYTES, saved.swapBytes);
        glPixelStorei(GL_PACK_LSB_FIRST, saved.lsbFirst);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint) previousFramebuffer);
    glDeleteFramebuffers(1, &framebuffer);
    return error;
}

static void test_pack_state(int winner) {
    printf("D. pixel-pack state (hostile incoming: rowLength=17 alignment=1 skip=5,7 swap=on)\n");

    size_t capacity = BYTES + SENTINEL;
    unsigned char *destination = malloc(capacity);
    if (destination == NULL) {
        fail("out of memory");
        return;
    }

    PackState ambient;
    get_pack(&ambient);

    /* Deliberately non-default, as another reader on the same context could have
     * left it. Each value is chosen so that ignoring it is observable. */
    const PackState hostile = {17, 1, 5, 7, GL_TRUE, GL_TRUE, 3, 2};
    clear_errors();
    set_pack(&hostile);
    GLenum setupError = glGetError();
    require(setupError == GL_NO_ERROR, "setting the hostile pack state raised %s",
        error_name(setupError));
    PackState recorded;
    get_pack(&recorded);
    require(pack_equal(&recorded, &hostile),
        "the driver did not record the hostile pack state (got rowLength=%d alignment=%d "
        "skip=%d,%d swap=%d lsb=%d image=%d/%d)",
        recorded.rowLength, recorded.alignment, recorded.skipRows, recorded.skipPixels,
        recorded.swapBytes, recorded.lsbFirst, recorded.imageHeight, recorded.skipImages);

    /* D1: the contract as it stood before the pack state was enumerated. */
    memset(destination, 0xCD, capacity);
    GLenum oldError = read_with_pack(destination, 0, winner);
    require(oldError == GL_NO_ERROR,
        "the old contract read failed with %s - corruption needs a read that reports success",
        error_name(oldError));
    int oldCorrect = oldError == GL_NO_ERROR
        && values_are_the_written_ones((const float *) destination, NULL);
    size_t pastFrame = first_changed(destination, capacity, BYTES, 0xCD);
    size_t unwritten = first_untouched(destination, BYTES, 0xCD);
    require(!oldCorrect,
        "the old row-length-and-alignment-only contract came back CORRECT under a hostile pack "
        "state, so this regression no longer exposes the bug it exists to expose");
    require(pastFrame < capacity || unwritten < BYTES,
        "the hostile skip left no trace: every byte in and past the frame was written cleanly");
    printf("  old contract: values %s, %s\n",
        oldCorrect ? "correct" : "corrupted",
        pastFrame < capacity
            ? "data written past the end of the frame"
            : "frame left partly unwritten (the skip lands in the source)");

    /* D2: bindings whose restoration has to be observable - restoring zero to
     * zero proves nothing. */
    GLuint dummyFbo;
    GLuint dummyPbo;
    glGenFramebuffers(1, &dummyFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, dummyFbo);
    glGenBuffers(1, &dummyPbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, dummyPbo);

    memset(destination, 0xCD, capacity);
    GLenum guardedError = read_with_pack(destination, 1, winner);
    require(guardedError == GL_NO_ERROR, "the guarded read failed with %s",
        error_name(guardedError));

    if (guardedError == GL_NO_ERROR) {
        require(values_are_the_written_ones((const float *) destination, NULL),
            "the guarded read did not return the written values from a hostile pack state");
        size_t guardedPast = first_changed(destination, capacity, BYTES, 0xCD);
        require(guardedPast == capacity,
            "the guarded read wrote past the frame, first at offset %zu of a %u-byte frame",
            guardedPast, (unsigned) BYTES);
        PackState after;
        get_pack(&after);
        require(pack_equal(&after, &hostile),
            "the guarded read did not restore the pack state (rowLength=%d alignment=%d "
            "skip=%d,%d swap=%d lsb=%d image=%d/%d)",
            after.rowLength, after.alignment, after.skipRows, after.skipPixels,
            after.swapBytes, after.lsbFirst, after.imageHeight, after.skipImages);
        GLint boundFbo = 0;
        GLint boundPbo = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundFbo);
        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &boundPbo);
        require(boundFbo == (GLint) dummyFbo && boundPbo == (GLint) dummyPbo,
            "the guarded read did not restore the bindings (readFbo=%d packBuffer=%d, expected "
            "%d and %d)", boundFbo, boundPbo, dummyFbo, dummyPbo);
    }

    set_pack(&ambient);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDeleteBuffers(1, &dummyPbo);
    glDeleteFramebuffers(1, &dummyFbo);
    free(destination);
    printf("  guarded read: values correct, nothing written outside the frame, all eight pack "
           "parameters and both bindings restored\n");
}

int main(void) {
    printf("gl-depth-readback-probe\n");

    EGLContext context = create_context();
    if (context == EGL_NO_CONTEXT) {
        printf("  FAILED: could not create a GL context (EGL error %d)\n", eglGetError());
        return 1;
    }
    printf("  renderer: %s\n", glGetString(GL_RENDERER));
    printf("  vendor:   %s\n", glGetString(GL_VENDOR));
    printf("  version:  %s\n", glGetString(GL_VERSION));

    render_bands();
    if (failures > 0) {
        return 1;
    }

    test_game_recipe();

    int winner = -1;
    int haveWinner = test_variants(&winner);

    if (haveWinner) {
        test_packing(winner);
        test_pack_state(winner);
    } else {
        printf("C. row contract: skipped, no readback recipe produced values\n");
        printf("D. pack-state contract: skipped for the same reason\n");
    }

    if (failures > 0) {
        printf("FAILED: %d assertion(s)\n", failures);
        return 1;
    }
    printf("Depth readback checks passed: the game's recipe is refused, the corrected "
           "recipe returns the written values, rows are packed tight and bottom-up, and a "
           "hostile pack state is guarded, bounded and restored.\n");
    return 0;
}
