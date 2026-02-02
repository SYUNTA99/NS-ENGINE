# stdinを読み捨て
$null = $input | Out-Null

$projectDir = if ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { "C:\Users\nanat\Desktop\NS-ENGINE" }
$timeFile = Join-Path $projectDir ".claude\prompt_time.txt"
$notifyScript = Join-Path $projectDir ".claude\hooks\notify.ps1"

if (Test-Path $timeFile) {
    $startTimeStr = Get-Content $timeFile -Raw
    try {
        $startTime = [DateTime]::Parse($startTimeStr.Trim())
        $elapsed = (Get-Date) - $startTime

        if ($elapsed.TotalSeconds -ge 60) {
            & powershell -ExecutionPolicy Bypass -File $notifyScript
        }
    } catch {
        # parse failure
    }
}

exit 0
