param(
    [string]$BuildDir = '',
    [string]$OutDir = '',
    [string]$PackageName = '',
    [string]$ZipName = '',
    [string]$ReadmePath = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $repoRoot 'build' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $repoRoot 'out' }
if ([string]::IsNullOrWhiteSpace($PackageName)) {
    $PackageName = 'nosfix-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
}
if ([string]::IsNullOrWhiteSpace($ZipName)) { $ZipName = $PackageName }

$dll = Join-Path $BuildDir 'nosfix.dll'
$ini = Join-Path $BuildDir 'nosfix.ini'
$bat = Join-Path $PSScriptRoot 'nosfix-config.bat'
$gui = Join-Path $PSScriptRoot 'nosfix-config-gui.ps1'
foreach ($required in @($dll, $ini, $bat, $gui)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "required package input missing: $required"
    }
}

$packageDir = Join-Path $OutDir $PackageName
if (Test-Path -LiteralPath $packageDir) {
    Remove-Item -LiteralPath $packageDir -Recurse -Force
}
$scriptsDir = Join-Path $packageDir 'scripts'
New-Item -ItemType Directory -Force -Path $scriptsDir | Out-Null

Copy-Item -LiteralPath $dll -Destination (Join-Path $packageDir 'nosfix.dll') -Force
Copy-Item -LiteralPath $ini -Destination (Join-Path $packageDir 'nosfix.ini') -Force
Copy-Item -LiteralPath $bat -Destination (Join-Path $scriptsDir 'nosfix-config.bat') -Force
Copy-Item -LiteralPath $gui -Destination (Join-Path $scriptsDir 'nosfix-config-gui.ps1') -Force

$license = Join-Path $BuildDir 'MinHook-LICENSE.txt'
if (Test-Path -LiteralPath $license) {
    Copy-Item -LiteralPath $license -Destination (Join-Path $packageDir 'MinHook-LICENSE.txt') -Force
}
if (-not [string]::IsNullOrWhiteSpace($ReadmePath)) {
    if (-not (Test-Path -LiteralPath $ReadmePath)) {
        throw "readme missing: $ReadmePath"
    }
    $readmeName = Split-Path -Leaf $ReadmePath
    Copy-Item -LiteralPath $ReadmePath -Destination (Join-Path $packageDir $readmeName) -Force
}

$zipPath = Join-Path $OutDir ($ZipName + '.zip')
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -Force
Write-Output "package: $packageDir"
Write-Output "zip:     $zipPath"
Write-Output "scripts: nosfix-config.bat, nosfix-config-gui.ps1"
