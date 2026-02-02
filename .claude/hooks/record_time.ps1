# 現在時刻を記録
$projectDir = if ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { "C:\Users\nanat\Desktop\NS-ENGINE" }
$timeFile = Join-Path $projectDir ".claude\prompt_time.txt"
Get-Date -Format "o" | Out-File -FilePath $timeFile -Encoding UTF8 -NoNewline
exit 0
