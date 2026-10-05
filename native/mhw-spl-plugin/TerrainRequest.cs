namespace CrafterHunter.MHW;

/// <summary>
/// The on-demand request channel for the terrain self-check. Nothing casts a
/// ray on its own: a person or an agent drops a request file next to the
/// plugin's log and the next sample consumes it, so the first native call into
/// the game's collision routine happens while someone is watching for it rather
/// than a second after a world loads.
/// </summary>
public static class TerrainRequest
{
    public const string FolderName = "terrain";
    public const string CheckFileName = "check.request";

    /// <summary>Where the request file lives, relative to the game directory.</summary>
    public static string CheckPath(string pluginFolder) =>
        Path.Combine(pluginFolder, FolderName, CheckFileName);

    /// <summary>
    /// Take the pending request, if there is one. The file is deleted before
    /// the ray runs, so a request fires exactly once even if the cast is
    /// refused or the session never gets a world to cast against.
    /// </summary>
    public static bool ConsumeCheck(string pluginFolder)
    {
        var path = CheckPath(pluginFolder);
        if (!File.Exists(path))
        {
            return false;
        }

        File.Delete(path);
        return true;
    }

    /// <summary>Drop a pending request without casting.</summary>
    public static void Clear(string pluginFolder)
    {
        var path = CheckPath(pluginFolder);
        if (File.Exists(path))
        {
            File.Delete(path);
        }
    }
}