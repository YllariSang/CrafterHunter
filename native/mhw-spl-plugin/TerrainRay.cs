using System.Numerics;

namespace CrafterHunter.MHW;

/// <summary>
/// Pure geometry and policy behind the host terrain queries: the signature
/// table the adapter resolves once, the down-segment it casts for its own
/// ground-truth check, and the rule that decides whether a ray agreed with the
/// hunter's collision point. Nothing in here touches the game, so the test
/// project outside the game can check it.
/// </summary>
public static class TerrainRay
{
    /// <summary>
    /// The three states from <c>docs/host-queries.md</c>. There is no fourth:
    /// a failure that would let a stale or unverified answer through collapses
    /// into <see cref="State.Disabled"/> for the whole session.
    /// </summary>
    public enum State
    {
        /// <summary>The fingerprint or a signature failed; nothing is ever queried.</summary>
        Disabled,

        /// <summary>Enabled, but the collision singleton or the hunter is missing.</summary>
        Unavailable,

        /// <summary>Signatures resolved, singleton present, rays may run.</summary>
        Ready,
    }

    /// <summary>
    /// The executable the signatures were published against, hashed by
    /// <c>tools/verify-host-build.py</c> before the adapter resolves anything.
    /// </summary>
    public const string PinnedExecutableSha256 =
        "c2ebbbd2c49f216d484e31a5219bed419eb1e5e7d206d02cba040a3ab79d90ea";

    /// <summary>
    /// MHW units per metre, the divisor the camera and player payloads already
    /// apply. One source of it, so a ray segment and a proxy position cannot
    /// drift apart by a scale factor.
    /// </summary>
    public const float UnitsPerMetre = 100.0f;

    /// <summary>
    /// Byte signatures for build 421810 (Ver. 15.23.00), published by the
    /// MIT-licensed justbustin/minecraft-crossover-bridge and credited in
    /// <c>docs/prior-art.md</c>. No addresses appear here: <c>.text</c> is
    /// opaque on disk, so only the loaded image can be scanned, and a pattern
    /// that does not resolve disables terrain queries instead of guessing.
    /// </summary>
    public static readonly (string Name, string Pattern)[] Signatures =
    [
        (
            "sCollision::CheckSegment",
            "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 81 EC B0 01 00 00 48 8B F2 49 8B D9 33 D2 41 0F B6 F8"
        ),
        ("TriangleInfo::ctor", "33 D2 C7 41 08 FF FF FF FF 33 C0"),
        ("TriangleInfo::reset", "33 C0 C7 41 08 FF FF FF FF 48 89 41 20"),
        ("Param::ctor", "45 33 C9 48 8D 05 ?? ?? ?? ?? 48 89 01 44 89 89 BC 00 00 00"),
        ("Param::setAttr", "89 51 08 44 89 41 10 44 89 49 0C C6 81 F8 00 00 00 01 C3"),
        ("Param::dtor", "48 8D 05 ?? ?? ?? ?? 48 89 01 C3"),
        (
            "TriangleInfo::attr",
            "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 8B F2 48 8B D9 80 7C 0E 7E"
        ),
    ];

    /// <summary>
    /// The segment for one column of a sweep: from one metre above a point
    /// offset sideways from the hunter, down through the ground. Metres, like
    /// everything the adapter takes and returns; only the cast itself works in
    /// MHW units. The centre column is offset zero, which is the one compared
    /// against the hunter's own collision point.
    /// </summary>
    public static void SweepColumn(
        Vector3 positionMetres,
        Vector3 right,
        float offsetMetres,
        float startAboveMetres,
        float depthMetres,
        out Vector3 start,
        out Vector3 end)
    {
        var column = positionMetres + right * offsetMetres;
        start = column + new Vector3(0, startAboveMetres, 0);
        end = column - new Vector3(0, depthMetres, 0);
    }

    /// <summary>
    /// The rise per metre between two columns a metre apart. NaN when either
    /// column found no surface, because a slope averaged across a missing
    /// sample is a made-up number.
    /// </summary>
    public static float SlopePerMetre(float leftHeight, float rightHeight, float spacingMetres)
    {
        if (!float.IsFinite(leftHeight) || !float.IsFinite(rightHeight) || spacingMetres <= 0)
        {
            return float.NaN;
        }

        return (rightHeight - leftHeight) / spacingMetres;
    }

    /// <summary>
    /// Whether a ray's hit height landed where the hunter's collision point
    /// says the ground is. Non-finite input never agrees: a NaN would compare
    /// false against every tolerance and quietly look like a verdict.
    /// </summary>
    public static bool Agrees(
        float rayHitYMetres,
        float collisionYMetres,
        float toleranceMetres)
    {
        if (!float.IsFinite(rayHitYMetres) || !float.IsFinite(collisionYMetres))
        {
            return false;
        }

        return MathF.Abs(rayHitYMetres - collisionYMetres) <= toleranceMetres;
    }

    /// <summary>
    /// The state machine. A failed fingerprint or an unresolved signature wins
    /// over everything, including a present singleton, so the session stays
    /// disabled no matter what the world does later.
    /// </summary>
    public static State Classify(
        bool fingerprintMatched,
        bool signaturesResolved,
        bool collisionSingletonPresent,
        bool hunterPresent)
    {
        if (!fingerprintMatched || !signaturesResolved)
        {
            return State.Disabled;
        }

        return collisionSingletonPresent && hunterPresent ? State.Ready : State.Unavailable;
    }
}
