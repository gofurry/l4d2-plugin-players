$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$configPath = Join-Path $PSScriptRoot "config.local.ps1"
$binaryPath = ""
if (Test-Path -LiteralPath $configPath -PathType Leaf) {
    . $configPath
    if (Get-Variable -Name GameServerBinaryPath -ErrorAction SilentlyContinue) {
        $binaryPath = $GameServerBinaryPath
    }
}

$arguments = @((Join-Path $PSScriptRoot "validate.py"), $projectRoot)
if (-not [string]::IsNullOrWhiteSpace($binaryPath)) {
    if (-not (Test-Path -LiteralPath $binaryPath -PathType Leaf)) {
        throw "Configured GameServerBinaryPath does not exist: $binaryPath"
    }
    $arguments += $binaryPath
}

& python (Join-Path $PSScriptRoot "test_validate.py")
if ($LASTEXITCODE -ne 0) {
    throw "Validation tests failed. Exit code: $LASTEXITCODE"
}

& python @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Validation failed. Exit code: $LASTEXITCODE"
}
