param(
    [Parameter(Mandatory = $true)][string]$Path
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Path)) {
    throw "log not found: $Path"
}

$lines = Get-Content -LiteralPath $Path
$match = {
    param($pattern)
    @($lines | Where-Object { $_ -match $pattern })
}

$judgeBefore = & $match 'PROBE judge before'
$judgeAfter = & $match 'PROBE judge after'
$motion = & $match 'PROBE motion'
$input = & $match 'PROBE input'
$outcome = & $match 'SHADOW outcome'
$effect = & $match 'SHADOW effect'
$live = & $match '5s SHADOW live'
$clock = & $match '5s shadow clock'

'=== startup ==='
$lines | Where-Object {
    $_ -match 'nosfix 0\.4\.0 initialized|diagnostic PROBE enabled|SHADOW live feed|shadow song clock publication'
} | Select-Object -First 12

'=== probe counts ==='
"judge before/after : {0}/{1}" -f $judgeBefore.Count, $judgeAfter.Count
"motion             : {0}" -f $motion.Count
"input slot         : {0}" -f $input.Count

'=== shadow feed ==='
"outcome lines      : {0}" -f $outcome.Count
"effect lines       : {0}" -f $effect.Count
if ($live.Count -gt 0) {
    'last 5s block:'
    $live | Select-Object -Last 1
} else {
    'no 5s SHADOW live line'
}

'=== input clock ==='
if ($clock.Count -gt 0) {
    'last 5s block:'
    $clock | Select-Object -Last 1
} else {
    'no 5s shadow clock line (native_shadow/capture_input may be off)'
}

'=== warnings/errors ==='
$bad = @($lines | Where-Object { $_ -match '\[(WARN|ERROR)\]' })
if ($bad.Count -eq 0) {
    'none'
} else {
    $bad | Select-Object -First 20
    "total: $($bad.Count)"
}

'=== last SHADOW outcome/effect ==='
@($outcome + $effect) | Select-Object -Last 12
