$ErrorActionPreference = "Stop"

$configPath = Join-Path $PSScriptRoot "config.local.ps1"
if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) {
    throw "Missing scripts\\config.local.ps1. Copy config.example.ps1 and configure local paths."
}
. $configPath

$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceFile = Join-Path $projectRoot "plugin\\src\\l4d2_players.sp"
$projectInclude = Join-Path $projectRoot "plugin\\include"
$gameData = Join-Path $projectRoot "gamedata\\l4d2_players.txt"
$distDirectory = Join-Path $projectRoot "dist"
$outputFile = Join-Path $distDirectory "l4d2_players.smx"

foreach ($requiredPath in @($CompilerPath, $SourceModInclude, $sourceFile, $projectInclude, $gameData)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required build path not found: $requiredPath"
    }
}

$compilerBanner = (& $CompilerPath 2>&1 | Out-String)
$versionMatch = [regex]::Match($compilerBanner, "SourcePawn Compiler (?<version>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)")
if (-not $versionMatch.Success) { throw "Unable to determine SourcePawn compiler version." }
if ($versionMatch.Groups["version"].Value -ne $ExpectedSourcePawnVersion) {
    throw "SourcePawn compiler version $($versionMatch.Groups['version'].Value) does not match required version $ExpectedSourcePawnVersion."
}

New-Item -ItemType Directory -Path $distDirectory -Force | Out-Null
Write-Host "Using SourcePawn Compiler $ExpectedSourcePawnVersion."
& $CompilerPath $sourceFile "-i$SourceModInclude" "-i$projectInclude" "-o$outputFile"
if ($LASTEXITCODE -ne 0) { throw "Compilation failed. spcomp exit code: $LASTEXITCODE" }
if (-not (Test-Path -LiteralPath $outputFile -PathType Leaf)) { throw "Compiler did not create output: $outputFile" }
Write-Host "Build succeeded: $outputFile"
