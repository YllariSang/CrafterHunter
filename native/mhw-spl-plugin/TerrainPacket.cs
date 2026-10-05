using System.Buffers.Binary;
using System.Numerics;

namespace CrafterHunter.MHW;

/// <summary>
/// The wire layout of the two terrain packets, in metres and host coordinates.
///
/// These bytes are pinned by <c>cargo test</c> in
/// <c>crafterhunter-protocol</c> as well as by the plugin's own tests: the guest
/// writes a request in Java and this reads it in C#, and this writes a result
/// that the guest reads. Every refusal here is a refusal to act on a payload
/// that does not match the pinned layout — no defaults, no partial reads.
/// </summary>
public static class TerrainPacket
{
    public const int RequestPayloadLength = 28;
    public const int ResultPayloadLength = 36;

    /// <summary>The adapter is disabled, or the singleton or hunter is missing.</summary>
    public const byte StatusNoTerrain = 0;

    /// <summary>The segment touched no surface. A real answer, not a missing one.</summary>
    public const byte StatusMiss = 1;

    public const byte StatusHit = 2;

    /// <summary>
    /// Read a request: a request id and the two segment endpoints. A wrong
    /// length or a non-finite endpoint is refused, because a segment with a NaN
    /// in it would be cast by the game and the answer would be meaningless.
    /// </summary>
    public static bool TryReadRequest(
        ReadOnlySpan<byte> payload,
        out uint id,
        out Vector3 start,
        out Vector3 end)
    {
        id = 0;
        start = Vector3.Zero;
        end = Vector3.Zero;
        if (payload.Length != RequestPayloadLength)
        {
            return false;
        }

        if (!IsFinite(ReadVector(payload, 4)) || !IsFinite(ReadVector(payload, 16)))
        {
            return false;
        }

        id = BinaryPrimitives.ReadUInt32LittleEndian(payload);
        start = ReadVector(payload, 4);
        end = ReadVector(payload, 16);
        return true;
    }

    /// <summary>
    /// Write a result. Position and normal are zeroed unless the status is a
    /// hit, so a miss cannot be read as a hit at the origin, and the three bytes
    /// after the status stay zero, matching the reserved bits in the header.
    /// </summary>
    public static byte[] EncodeResult(
        uint id,
        byte status,
        Vector3 position,
        Vector3 normal,
        uint attribute)
    {
        var payload = new byte[ResultPayloadLength];
        BinaryPrimitives.WriteUInt32LittleEndian(payload, id);
        payload[4] = status;
        if (status == StatusHit)
        {
            WriteVector(payload.AsSpan(8), position);
            WriteVector(payload.AsSpan(20), normal);
            BinaryPrimitives.WriteUInt32LittleEndian(payload.AsSpan(32), attribute);
        }

        return payload;
    }

    private static Vector3 ReadVector(ReadOnlySpan<byte> payload, int offset) =>
        new(
            BinaryPrimitives.ReadSingleLittleEndian(payload.Slice(offset, 4)),
            BinaryPrimitives.ReadSingleLittleEndian(payload.Slice(offset + 4, 4)),
            BinaryPrimitives.ReadSingleLittleEndian(payload.Slice(offset + 8, 4)));

    private static void WriteVector(Span<byte> payload, Vector3 value)
    {
        BinaryPrimitives.WriteSingleLittleEndian(payload, value.X);
        BinaryPrimitives.WriteSingleLittleEndian(payload.Slice(4, 4), value.Y);
        BinaryPrimitives.WriteSingleLittleEndian(payload.Slice(8, 4), value.Z);
    }

    private static bool IsFinite(Vector3 value) =>
        float.IsFinite(value.X) && float.IsFinite(value.Y) && float.IsFinite(value.Z);
}