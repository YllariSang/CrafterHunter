param(
    [Parameter(Mandatory = $true)]
    [string]$GameDirectory
)

$ErrorActionPreference = "Stop"
$resolvedDirectory = (Resolve-Path -LiteralPath $GameDirectory).Path
$executable = Join-Path $resolvedDirectory "MonsterHunterWorld.exe"

if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "MonsterHunterWorld.exe was not found in: $resolvedDirectory"
}

$file = Get-Item -LiteralPath $executable
$hash = Get-FileHash -LiteralPath $executable -Algorithm SHA256

[PSCustomObject]@{
    Path           = $file.FullName
    FileVersion    = $file.VersionInfo.FileVersion
    ProductVersion = $file.VersionInfo.ProductVersion
    SizeBytes      = $file.Length
    LastWriteUtc   = $file.LastWriteTimeUtc.ToString("o")
    SHA256         = $hash.Hash
} | Format-List

Write-Host "Share this text output only. Do not upload the executable."
