param(
    [Parameter(Mandatory=$true)][string]$TaskDirectory,
    [Parameter(Mandatory=$true)][string]$PromptFile
)
$ErrorActionPreference = 'Stop'
$workerRoot = (Resolve-Path -LiteralPath $TaskDirectory).Path
$promptPath = (Resolve-Path -LiteralPath $PromptFile).Path
Push-Location -LiteralPath $workerRoot
try {
    # Prompt is data on stdin, never interpolated into shell command text.
    $promptText = Get-Content -Raw -LiteralPath $promptPath
    # Windows PowerShell treats native stderr as an error record. Preserve
    # diagnostics without terminating a healthy worker on a warning.
    $ErrorActionPreference = 'Continue'
    $promptText | & 'D:\Share\meka.exe' --profile deepseek --oneshot --no-stream --format json --permission workspace --sandbox-backend restricted-token --additional-readable-directory 'D:\Share\nosfix' --additional-readable-directory 'D:\Share\chart-requester\reference\spice2x\src\spice2x' --prompt - 1> 'worker-output.json' 2> 'worker-stderr.txt'
    $workerExit = $LASTEXITCODE
    Set-Content -LiteralPath 'worker-exit.txt' -Value $workerExit -Encoding ASCII
    Write-Output "meka worker $workerRoot exited $workerExit"
    if ($workerExit -ne 0) { exit $workerExit }
} finally {
    Pop-Location
}
