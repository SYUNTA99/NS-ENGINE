# OpenSpec sync helper
# Show current openspec state to assist with spec feedback decision
# Usage: .\openspec-feedback.ps1

$specsDir = "openspec/specs"
$changesDir = "openspec/changes"

Write-Host "=== OpenSpec Sync Helper ==="
Write-Host ""

# Show existing specs
if (Test-Path $specsDir) {
    $specs = Get-ChildItem -Path $specsDir -Directory -ErrorAction SilentlyContinue
    if ($specs) {
        Write-Host "Existing specs:"
        foreach ($s in $specs) {
            $specFile = Join-Path $s.FullName "spec.md"
            if (Test-Path $specFile) {
                Write-Host "  - $($s.Name)"
            }
        }
    } else {
        Write-Host "No specs yet."
    }
} else {
    Write-Host "No openspec/specs/ directory."
}

Write-Host ""

# Show active changes
if (Test-Path $changesDir) {
    $changes = Get-ChildItem -Path $changesDir -Directory -ErrorAction SilentlyContinue
    if ($changes) {
        Write-Host "Active changes:"
        foreach ($c in $changes) {
            Write-Host "  - $($c.Name)"
        }
    } else {
        Write-Host "No active changes."
    }
} else {
    Write-Host "No openspec/changes/ directory."
}
