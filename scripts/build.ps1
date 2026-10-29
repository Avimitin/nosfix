param(
    [switch]$RunTests
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$sdkRoot = Join-Path $repoRoot '.tools\sdk'
$sdkVersion = '10.0.26100.0'
$sdkCpp = Join-Path $sdkRoot 'cpp\c'
$sdkX64 = Join-Path $sdkRoot 'x64\c'
$vcVars = Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path -LiteralPath $vswhere) {
    $candidate = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'VC\Auxiliary\Build\vcvars64.bat' | Select-Object -First 1
    if ($candidate) { $vcVars = $candidate }
}
if (-not (Test-Path -LiteralPath $vcVars)) {
    throw 'Visual Studio 2022 x64 C++ tools are required.'
}
if (-not (Test-Path -LiteralPath (Join-Path $sdkCpp "Include\$sdkVersion\um\Windows.h"))) {
    throw "The local Windows SDK headers are missing under $sdkRoot."
}

$sdkIncludes = @(
    "$(Join-Path $sdkCpp "Include\$sdkVersion\shared")",
    "$(Join-Path $sdkCpp "Include\$sdkVersion\um")",
    "$(Join-Path $sdkCpp "Include\$sdkVersion\ucrt")",
    "$(Join-Path $sdkCpp "Include\$sdkVersion\winrt")",
    "$(Join-Path $sdkCpp "Include\$sdkVersion\cppwinrt")"
) -join ';'
$sdkLibraries = @(
    (Join-Path $sdkX64 'um\x64'),
    (Join-Path $sdkX64 'ucrt\x64')
) -join ';'
$rc = Join-Path $sdkCpp "bin\$sdkVersion\x64\rc.exe"
$mt = Join-Path $sdkCpp "bin\$sdkVersion\x64\mt.exe"
foreach ($tool in @($rc, $mt)) {
    if (-not (Test-Path -LiteralPath $tool)) { throw "Missing SDK tool: $tool" }
}
# CMake writes the RC compiler into CMakeRCCompiler.cmake as an unquoted
# `set()` value, so a Windows path with backslashes fails to parse
# ("Invalid character escape '\S'"). Hand CMake forward slashes instead.
$rcCmake = $rc -replace '\\', '/'
$mtCmake = $mt -replace '\\', '/'

$buildScript = Join-Path $env:TEMP 'nosfix-build-env.cmd'
$scriptContents = @"
@echo off
call "$vcVars" >nul
if errorlevel 1 exit /b %errorlevel%
set "INCLUDE=%INCLUDE%;$sdkIncludes"
set "LIB=%LIB%;$sdkLibraries"
set "PATH=%PATH%;$(Split-Path -Parent $rc)"
uv tool run --from cmake cmake -S "$repoRoot" -B "$repoRoot\build" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_RC_COMPILER="$rcCmake" -DCMAKE_MT="$mtCmake"
if errorlevel 1 exit /b %errorlevel%
uv tool run --from cmake cmake --build "$repoRoot\build" --config Release
if errorlevel 1 exit /b %errorlevel%
"@
Set-Content -LiteralPath $buildScript -Value $scriptContents -Encoding ASCII
& $env:ComSpec /d /c $buildScript
if ($LASTEXITCODE -ne 0) { throw "nosfix build failed with exit code $LASTEXITCODE" }

if ($RunTests) {
    & uv tool run --from cmake ctest --test-dir (Join-Path $repoRoot 'build') --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "nosfix tests failed with exit code $LASTEXITCODE" }
}

Write-Output "Built $repoRoot\build\nosfix.dll"
