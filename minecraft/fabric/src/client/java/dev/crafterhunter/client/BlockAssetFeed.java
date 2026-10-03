package dev.crafterhunter.client;

import com.mojang.blaze3d.platform.NativeImage;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import net.minecraft.client.Minecraft;
import net.minecraft.resources.Identifier;
import net.minecraft.server.packs.resources.ResourceManager;

/** Sends pixels from Minecraft's active resource pack, never a bundled imitation. */
public final class BlockAssetFeed {
    private static final String BLOCK_ID = "minecraft:stone";
    private static final Identifier TEXTURE = Identifier.fromNamespaceAndPath(
        "minecraft", "textures/block/stone.png"
    );
    private static volatile Asset latest;
    private static ResourceManager loadedFrom;
    private static long nextRetryNanos;

    private BlockAssetFeed() {
    }

    public static Asset latest() {
        return latest;
    }

    /** Called on Minecraft's client thread after the resource manager is ready. */
    public static void refresh(Minecraft minecraft) {
        if (minecraft.level == null) return;
        ResourceManager manager = minecraft.getResourceManager();
        if (manager == loadedFrom) return;
        if (System.nanoTime() < nextRetryNanos) return;
        latest = null;
        try (InputStream stream = manager.open(TEXTURE)) {
            byte[] png = stream.readAllBytes();
            byte[] id = BLOCK_ID.getBytes(StandardCharsets.UTF_8);
            if (png.length + id.length + 1 > Protocol.MAX_PAYLOAD_LENGTH) {
                throw new IOException("stone texture PNG exceeds bridge packet limit");
            }
            try (NativeImage image = NativeImage.read(png)) {
                if (image.getWidth() != 16 || image.getHeight() != 16) {
                    throw new IOException("stone texture must be 16x16 for this comparison");
                }
                byte[] pixels = new byte[1 + id.length + 2 + 16 * 16 * 4];
                pixels[0] = (byte) id.length;
                System.arraycopy(id, 0, pixels, 1, id.length);
                pixels[1 + id.length] = 16;
                pixels[2 + id.length] = 16;
                int offset = 3 + id.length;
                for (int y = 0; y < 16; y++) {
                    for (int x = 0; x < 16; x++) {
                        int argb = image.getPixel(x, y);
                        pixels[offset++] = (byte) (argb >>> 16);
                        pixels[offset++] = (byte) (argb >>> 8);
                        pixels[offset++] = (byte) argb;
                        pixels[offset++] = (byte) (argb >>> 24);
                    }
                }
                byte[] pngPacket = new byte[1 + id.length + png.length];
                pngPacket[0] = (byte) id.length;
                System.arraycopy(id, 0, pngPacket, 1, id.length);
                System.arraycopy(png, 0, pngPacket, 1 + id.length, png.length);
                latest = new Asset(BLOCK_ID, pixels, pngPacket);
                loadedFrom = manager;
                System.out.println("[CrafterHunter] Exporting actual " + BLOCK_ID +
                    " texture from active Minecraft resources (" + png.length + " PNG bytes)");
            }
        } catch (IOException error) {
            nextRetryNanos = System.nanoTime() + 2_000_000_000L;
            System.err.println("[CrafterHunter] Block asset unavailable: " + error.getMessage());
        }
    }

    public record Asset(String blockId, byte[] pixelsPacket, byte[] pngPacket) {
    }
}
