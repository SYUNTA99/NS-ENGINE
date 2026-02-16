# OpenSpec コンテキスト読み込み
# タスク開始時に関連する spec/change を検出し、要約を出力する
# Usage: .\openspec-context.ps1 [module-name]

param(
    [string]$Module = ""
)

$specsDir = "openspec/specs"
$changesDir = "openspec/changes"
$found = $false

Write-Host "=== OpenSpec Context Check ==="
Write-Host ""

# specs チェック
if (Test-Path $specsDir) {
    $specs = Get-ChildItem -Path $specsDir -Directory -ErrorAction SilentlyContinue
    if ($specs) {
        if ($Module) {
            $specs = $specs | Where-Object { $_.Name -like "*$Module*" }
        }
        if ($specs) {
            Write-Host "## Specs found:"
            foreach ($s in $specs) {
                $specFile = Join-Path $s.FullName "spec.md"
                if (Test-Path $specFile) {
                    Write-Host "  - $($s.Name): $specFile"
                    $found = $true
                }
            }
        }
    }
}

if (-not $found) {
    Write-Host "No specs found."
}

Write-Host ""

# changes チェック
$changesFound = $false
if (Test-Path $changesDir) {
    $changes = Get-ChildItem -Path $changesDir -Directory -ErrorAction SilentlyContinue
    if ($changes) {
        Write-Host "## Active changes:"
        foreach ($c in $changes) {
            Write-Host "  - $($c.Name)"
            $changesFound = $true
        }
    }
}

if (-not $changesFound) {
    Write-Host "No active changes."
}

Write-Host ""

if ($found -or $changesFound) {
    Write-Host "OPENSPEC_RELEVANT=true"
} else {
    Write-Host "OPENSPEC_RELEVANT=false"
}
