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
 * Four things are measured rather than argued, because each of them has a wrong
 * answer that produces a plausible image:
 *
 *   A. the game's own recipe must fail with GL_INVALID_FRAMEBUFFER_OPERATION.
 *      If it ever stops failing, the diagnosis this file records is out of date
 *      and the test says so instead of passing quietly.
 *   B. which read-buffer setting makes a depth-only framebuffer readable, and
 *      whether the values that come back are the ones that were written.
 *   C. the row contract: rows are width*4 bytes, the first row is the
 *      framebuffer's bottom row, and a larger destination buffer does not change
 *      either. Buffer capacity is not row stride, and a stride that is assumed
 *      instead of measured shears every row after the first.
 *   D. the D32_FLOAT values survive the round trip exactly (0.25f and 0.75f are
 *      both exactly representable, so equality is exact, not approximate).
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
    } else {
        printf("C. row contract: skipped, no readback recipe produced values\n");
    }

    if (failures > 0) {
        printf("FAILED: %d assertion(s)\n", failures);
        return 1;
    }
    printf("Depth readback checks passed: the game's recipe is refused, the corrected "
           "recipe returns the written values, rows are packed tight and bottom-up.\n");
    return 0;
}
