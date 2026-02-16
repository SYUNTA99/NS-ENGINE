$input = [Console]::In.ReadToEnd() | ConvertFrom-Json
$path = $input.tool_input.file_path

if ($path -and ($path -replace '\\','/' | Select-String -Pattern '\.claude/CLAUDE\.md$' -Quiet)) {
    @{ decision = "block"; reason = "CLAUDE.md is protected. Tell the user: CLAUDE.md is protected by a hook. If you want me to edit it, re-run with the exact phrase 'CLAUDE.mdを編集して' in your message." } | ConvertTo-Json -Compress
} else {
    @{ decision = "approve" } | ConvertTo-Json -Compress
}
