$ErrorActionPreference = "Stop"

$version = "0.3.0"
$projectRoot = (Resolve-Path (Split-Path -Parent $PSScriptRoot)).Path
$artifactRoot = Join-Path $projectRoot ".artifacts"
$stage = Join-Path $artifactRoot "l4d2-plugin-players-v$version"
$dist = Join-Path $projectRoot "dist"
$archive = Join-Path $dist "l4d2-plugin-players-v$version.zip"

& (Join-Path $PSScriptRoot "build.ps1")
& (Join-Path $PSScriptRoot "validate.ps1")

$resolvedArtifactParent = [System.IO.Path]::GetFullPath($artifactRoot)
$resolvedProject = [System.IO.Path]::GetFullPath($projectRoot)
if (-not $resolvedArtifactParent.StartsWith($resolvedProject, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Artifact directory escaped the project root: $resolvedArtifactParent"
}
if (Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null

$smRoot = Join-Path $stage "left4dead2\\addons\\sourcemod"
$pluginDirectory = Join-Path $smRoot "plugins"
$gamedataDirectory = Join-Path $smRoot "gamedata"
$translationDirectory = Join-Path $smRoot "translations"
New-Item -ItemType Directory -Path $pluginDirectory, $gamedataDirectory, $translationDirectory -Force | Out-Null

Copy-Item -LiteralPath (Join-Path $dist "l4d2_players.smx") -Destination $pluginDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot "gamedata\\l4d2_players.txt") -Destination $gamedataDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot "translations\\l4d2_players.phrases.txt") -Destination $translationDirectory
foreach ($document in @("README.md", "INSTALL.zh-CN.md", "CHANGELOG.md", "LICENSE")) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $document) -Destination $stage
}

if (Test-Path -LiteralPath $archive) {
    Remove-Item -LiteralPath $archive -Force
}
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $archive -CompressionLevel Optimal

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($archive)
try {
    $entries = @($zip.Entries | ForEach-Object { $_.FullName.Replace("\\", "/") })
    $required = @(
        "left4dead2/addons/sourcemod/plugins/l4d2_players.smx",
        "left4dead2/addons/sourcemod/gamedata/l4d2_players.txt",
        "left4dead2/addons/sourcemod/translations/l4d2_players.phrases.txt",
        "README.md", "INSTALL.zh-CN.md", "CHANGELOG.md", "LICENSE"
    )
    foreach ($entry in $required) {
        if ($entries -notcontains $entry) { throw "Release archive is missing: $entry" }
    }
    if ($entries | Where-Object { $_ -match "(^|/)(plugin|scripts|docs|contracts)/" }) {
        throw "Release archive unexpectedly contains development sources."
    }
}
finally {
    $zip.Dispose()
}

Write-Host "Package succeeded: $archive"

