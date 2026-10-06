package dev.crafterhunter.client;

import com.mojang.blaze3d.buffers.GpuBuffer;
import com.mojang.blaze3d.opengl.GlBuffer;
import com.mojang.blaze3d.opengl.GlStateManager;
import com.mojang.blaze3d.opengl.GlTexture;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.textures.GpuTexture;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL11C;
import org.lwjgl.opengl.GL21;
import org.lwjgl.opengl.GL30;
import org.lwjgl.opengl.GL30C;

/**
 * Reads a depth attachment that the game's own readback path refuses to read.
 *
 * <p>{@code GlCommandEncoder.copyTextureToBuffer} binds the source texture through
 * {@code bindFrameBufferTextures(readFbo, texture, 0, level, GL_READ_FRAMEBUFFER)},
 * and that overload puts the texture on <b>GL_COLOR_ATTACHMENT0</b> while the depth
 * slot receives {@code 0}. A D32_FLOAT image on a colour attachment is not a
 * complete framebuffer, so its {@code glReadPixels} is refused with
 * {@code GL_INVALID_FRAMEBUFFER_OPERATION} - which is exactly the one line the live
 * game log carried:
 *
 * <pre>OpenGL debug message: ... type=ERROR ... message='GL_INVALID_FRAMEBUFFER_OPERATION
 * in glReadPixels(incomplete framebuffer)'</pre>
 *
 * <p>The same class already binds depth correctly when it blits
 * ({@code copyTextureToTexture} passes the texture as the depth argument when
 * {@code hasDepthAspect()} is true), so this is an omission in the readback path
 * rather than a limit of the backend. What replaces it here attaches the depth image
 * to <b>GL_DEPTH_ATTACHMENT</b> and selects {@code GL_NONE} as the read buffer.
 *
 * <p>Both facts - the refusal and the corrected recipe - are measured on this
 * machine's own drivers, on the game's own driver included, by
 * {@code tools/test-depth-readback.sh}. That probe is what stops this comment from
 * being an argument: if a driver ever accepts the game's recipe, or refuses this
 * one, the probe fails and this file is out of date rather than merely wrong.
 *
 * <p>Nothing here waits. The read is issued, its completion is queued behind a fence
 * the way the colour half's is, and the caller decides later whether the pixels are
 * worth reading. A refused read is reported, never retried: the caller fails closed.
 */
public final class DepthReadback {

    /**
     * Outcome of one read.
     *
     * @param ok      whether the read was issued and a completion was queued
     * @param detail  what was observed, in either case - this is what the capture
     *                metadata and the failure status both quote
     */
    public record Result(boolean ok, String detail) {
    }

    private DepthReadback() {
    }

    /**
     * Issues a whole-frame depth read into {@code destination}.
     *
     * <p>The framebuffer this creates is bound only for the duration of the call and
     * is deleted before returning, so nothing keeps a handle on the game's depth
     * texture - which the game may reallocate on the next resize. Every piece of
     * global GL state touched (the read framebuffer binding, the pixel-pack buffer
     * binding, and all six pack parameters the read depends on) is read first and
     * put back, because the game's own encoder caches the framebuffer binding and a
     * stale cache would make it skip a bind it believes it has already done - and
     * because a pack state left changed is a defect that surfaces in some later,
     * unrelated read rather than in this one.
     *
     * @param completion runs when the GPU has finished writing the buffer, not when
     *                   the call returns
     */
    public static Result read(GpuTexture texture, GpuBuffer destination, int width, int height,
            Runnable completion) {
        if (!(texture instanceof GlTexture source) || !(destination instanceof GlBuffer target)) {
            return new Result(false, "not the OpenGL backend: texture="
                + texture.getClass().getName() + " buffer=" + destination.getClass().getName());
        }
        if (source.isClosed()) {
            return new Result(false, "depth texture is closed: " + source.getLabel());
        }
        if (!source.getFormat().hasDepthAspect()) {
            return new Result(false, "format " + source.getFormat() + " has no depth aspect");
        }
        long written = DepthReadbackPlan.writtenBytes(width, height, DepthReadbackPlan.PACK_ALIGNMENT);
        if (written <= 0 || destination.size() < written) {
            // Capacity is checked against what the read writes, not against what the
            // buffer happens to be called: a short buffer shears the last rows.
            return new Result(false, "buffer holds " + destination.size() + " of " + written
                + " bytes for " + width + "x" + height);
        }

        int previousFramebuffer = GlStateManager.getFrameBuffer(GL30.GL_READ_FRAMEBUFFER);
        int previousPackBuffer = GL11C.glGetInteger(GL21.GL_PIXEL_PACK_BUFFER_BINDING);
        int previousRowLength = GL11C.glGetInteger(GL11.GL_PACK_ROW_LENGTH);
        int previousAlignment = GL11C.glGetInteger(GL11.GL_PACK_ALIGNMENT);
        int previousSkipRows = GL11C.glGetInteger(GL11.GL_PACK_SKIP_ROWS);
        int previousSkipPixels = GL11C.glGetInteger(GL11.GL_PACK_SKIP_PIXELS);
        int previousSwapBytes = GL11C.glGetInteger(GL11.GL_PACK_SWAP_BYTES);
        int previousLsbFirst = GL11C.glGetInteger(GL11.GL_PACK_LSB_FIRST);
        // What the read would have inherited. Reported in the metadata even when it
        // is about to be overwritten, because "the read succeeded" and "the read
        // succeeded despite a hostile pack state" are different findings, and only
        // one of them is recorded if the difference is noticed after the fact.
        String observedPack = DepthReadbackPlan.nonDefaultPack(previousRowLength, previousAlignment,
            previousSkipRows, previousSkipPixels, previousSwapBytes != 0, previousLsbFirst != 0);

        int framebuffer = 0;
        try {
            framebuffer = GlStateManager.glGenFramebuffers();
            GlStateManager._glBindFramebuffer(GL30.GL_READ_FRAMEBUFFER, framebuffer);
            GlStateManager._glFramebufferTexture2D(GL30.GL_READ_FRAMEBUFFER,
                GL30.GL_DEPTH_ATTACHMENT, GL11.GL_TEXTURE_2D, source.glId(), 0);
            GlStateManager._glFramebufferTexture2D(GL30.GL_READ_FRAMEBUFFER,
                GL30.GL_COLOR_ATTACHMENT0, GL11.GL_TEXTURE_2D, 0, 0);
            // With no colour attachment there is no colour buffer to read from, and
            // GL_NONE is the only read buffer that says so. Measured, not assumed:
            // glReadBuffer(GL_DEPTH_ATTACHMENT) is not a legal value and the default
            // (GL_COLOR_ATTACHMENT0) only happens to work on the drivers tested.
            GL11C.glReadBuffer(GL11.GL_NONE);

            int status = GL30C.glCheckFramebufferStatus(GL30.GL_READ_FRAMEBUFFER);
            if (status != GL30.GL_FRAMEBUFFER_COMPLETE) {
                // Fail closed before issuing anything: a refused glReadPixels would
                // leave the same "no pixels" state with a less precise reason.
                return new Result(false, "read framebuffer is not complete: "
                    + DepthReadbackPlan.framebufferStatusName(status));
            }

            GlStateManager.clearGlErrors();
            GlStateManager._glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER, target.handle());
            // The whole pack state, not just the two parameters the arithmetic uses.
            // Setting row length and alignment alone leaves a previous writer's skip
            // and byte-swap in force: skip moves the read past the end of a buffer
            // sized exactly to the frame, and byte-swap silently reverses every
            // float. Neither raises an error; both destroy every value. Measured in
            // tools/test-depth-readback.sh section D, which starts from that state
            // and shows this read come back correct and in bounds.
            GlStateManager._pixelStore(GL11.GL_PACK_ROW_LENGTH, width);
            GlStateManager._pixelStore(GL11.GL_PACK_ALIGNMENT, DepthReadbackPlan.PACK_ALIGNMENT);
            GlStateManager._pixelStore(GL11.GL_PACK_SKIP_ROWS, DepthReadbackPlan.PACK_SKIP_ROWS);
            GlStateManager._pixelStore(GL11.GL_PACK_SKIP_PIXELS, DepthReadbackPlan.PACK_SKIP_PIXELS);
            GlStateManager._pixelStore(GL11.GL_PACK_SWAP_BYTES,
                DepthReadbackPlan.PACK_SWAP_BYTES ? 1 : 0);
            GlStateManager._pixelStore(GL11.GL_PACK_LSB_FIRST,
                DepthReadbackPlan.PACK_LSB_FIRST ? 1 : 0);
            GlStateManager._readPixels(0, 0, width, height, GL11.GL_DEPTH_COMPONENT, GL11.GL_FLOAT, 0L);
            int error = GlStateManager._getError();
            if (error != 0) {
                return new Result(false, "glReadPixels failed: " + DepthReadbackPlan.glErrorName(error));
            }

            // Queued, not invoked: this is a pixel-pack buffer read, so the bytes are
            // not necessarily there when the call returns. Completion has to mean the
            // GPU reached the read, or the caller would map a buffer that is still
            // being written - plausible numbers again.
            RenderSystem.queueFencedTask(completion);

            return new Result(true, "complete format=" + source.getFormat() + " " + width + "x"
                + height + " rowStride=" + DepthReadbackPlan.rowStrideBytes(width)
                + " rowsPadded=" + DepthReadbackPlan.rowsPadded(width, width,
                    DepthReadbackPlan.PACK_ALIGNMENT)
                + " packNonDefault=" + (observedPack.isEmpty() ? "none" : observedPack));
        } finally {
            GlStateManager._glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER, previousPackBuffer);
            GlStateManager._pixelStore(GL11.GL_PACK_ROW_LENGTH, previousRowLength);
            GlStateManager._pixelStore(GL11.GL_PACK_ALIGNMENT, previousAlignment);
            GlStateManager._pixelStore(GL11.GL_PACK_SKIP_ROWS, previousSkipRows);
            GlStateManager._pixelStore(GL11.GL_PACK_SKIP_PIXELS, previousSkipPixels);
            GlStateManager._pixelStore(GL11.GL_PACK_SWAP_BYTES, previousSwapBytes);
            GlStateManager._pixelStore(GL11.GL_PACK_LSB_FIRST, previousLsbFirst);
            GlStateManager._glBindFramebuffer(GL30.GL_READ_FRAMEBUFFER, previousFramebuffer);
            if (framebuffer != 0) {
                GlStateManager._glDeleteFramebuffers(framebuffer);
            }
        }
    }
}
