using System.Text;

namespace CrafterHunter.MHW;

// The block ID and pixels arrive from Minecraft's active resource manager.
// The mod never embeds or ships a copy of Mojang's texture in this repository.
internal sealed record MinecraftBlockAsset(string BlockId, byte[] Rgba, byte[] Png)
{
    internal const string ExpectedBlockId = "minecraft:stone";
    internal const int Size = 16;
    private static ReadOnlySpan<byte> PngSignature => [137, 80, 78, 71, 13, 10, 26, 10];

    internal static bool TryReadPixels(ReadOnlySpan<byte> payload, out byte[] rgba)
    {
        rgba = [];
        if (!TryReadId(payload, out var offset) ||
            payload.Length != offset + 2 + Size * Size * 4 ||
            payload[offset] != Size || payload[offset + 1] != Size)
        {
            return false;
        }
        rgba = payload[(offset + 2)..].ToArray();
        return true;
    }

    internal static bool TryReadPng(ReadOnlySpan<byte> payload, out byte[] png)
    {
        png = [];
        if (!TryReadId(payload, out var offset) ||
            payload.Length <= offset + PngSignature.Length ||
            !payload[offset..].StartsWith(PngSignature))
        {
            return false;
        }
        png = payload[offset..].ToArray();
        return true;
    }

    private static bool TryReadId(ReadOnlySpan<byte> payload, out int offset)
    {
        offset = 0;
        if (payload.Length < 2 || payload[0] == 0 || payload.Length < 1 + payload[0])
        {
            return false;
        }
        offset = 1 + payload[0];
        return Encoding.UTF8.GetString(payload.Slice(1, payload[0])) == ExpectedBlockId;
    }
}
