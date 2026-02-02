# Post tool use hook - format C++ files
try {
    $json = $Input | Out-String | ConvertFrom-Json
    $filePath = $json.tool_input.file_path

    if ($filePath -match '\.(cpp|h)$') {
        $clangFormat = "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin/clang-format.exe"
        if (Test-Path $clangFormat) {
            & $clangFormat -i --style=file:$env:CLAUDE_PROJECT_DIR/.claude/clang-format.yaml $filePath
        }
    }
} catch {
    # エラーを無視
}
exit 0
