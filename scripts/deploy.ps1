$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "config.local.ps1")
if ([string]::IsNullOrWhiteSpace($ServerRoot)) { throw "ServerRoot is not configured." }

$projectRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot "build.ps1")

$smRoot = Join-Path $ServerRoot "left4dead2\\addons\\sourcemod"
$targets = @(
    @{ Source = Join-Path $projectRoot "dist\\l4d2_players.smx"; Target = Join-Path $smRoot "plugins\\l4d2_players.smx" },
    @{ Source = Join-Path $projectRoot "gamedata\\l4d2_players.txt"; Target = Join-Path $smRoot "gamedata\\l4d2_players.txt" },
    @{ Source = Join-Path $projectRoot "translations\\l4d2_players.phrases.txt"; Target = Join-Path $smRoot "translations\\l4d2_players.phrases.txt" }
)
foreach ($item in $targets) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $item.Target) -Force | Out-Null
    Copy-Item -LiteralPath $item.Source -Destination $item.Target -Force
}
Write-Host "Deployed L4D2 Players to $smRoot"

