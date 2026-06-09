param(
    [ValidateSet("ninja-x86-debug", "ninja-x86-release")]
    [string]$Preset = "ninja-x86-release",

    [switch]$ConfigureOnly,
    [switch]$SkipEnvCheck,
    [switch]$EmitAsm
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installPath = $null

if (Test-Path -LiteralPath $vswhere) {
    $installPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ($installPath) {
        $installPath = $installPath.Trim()
    }
}

if (-not $installPath) {
    $fallbacks = @(
        "$env:ProgramFiles\Microsoft Visual Studio\18\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\18\Community",
        "$env:ProgramFiles\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community"
    )

    foreach ($candidate in $fallbacks) {
        if (Test-Path -LiteralPath (Join-Path $candidate "VC\Auxiliary\Build\vcvarsall.bat")) {
            $installPath = $candidate
            break
        }
    }
}

if (-not $installPath) {
    throw "Could not find a Visual Studio installation with x86 MSVC tools. Install MSVC x86 tools or run from an x86 MSVC Developer PowerShell."
}

$vcvarsall = Join-Path $installPath "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path -LiteralPath $vcvarsall)) {
    throw "vcvarsall.bat was not found at $vcvarsall"
}

$commands = @(
    "call `"$vcvarsall`" x86",
    "cd /d `"$repoRoot`""
)

if (-not $SkipEnvCheck) {
    $commands += "python tools\recoil.py env --native-x86"
}

$configureCommand = "cmake --preset $Preset"
if ($EmitAsm) {
    $configureCommand += " -DRECOIL_EMIT_ASM=ON"
}

$commands += $configureCommand

if (-not $ConfigureOnly) {
    $commands += "cmake --build --preset $Preset"
}

& $env:ComSpec /d /s /c ($commands -join " && ")
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
