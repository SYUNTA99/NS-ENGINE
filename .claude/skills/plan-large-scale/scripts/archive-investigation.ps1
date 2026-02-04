<#
.SYNOPSIS
    調査結果を計画ディレクトリにアーカイブする

.PARAMETER FeatureName
    機能名（計画ディレクトリ名）

.EXAMPLE
    .\archive-investigation.ps1 rhi
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$FeatureName
)

$ErrorActionPreference = "Stop"

$projectRoot = (Get-Item "$PSScriptRoot/../../../..").FullName
$currentDir = Join-Path $projectRoot ".claude/plans/current"
$featureDir = Join-Path $projectRoot ".claude/plans/$FeatureName"

# current/ が存在するか確認
if (-not (Test-Path $currentDir)) {
    Write-Error "current/ directory not found. Run init-investigation.ps1 first."
    exit 1
}

# current/findings.md が存在するか確認
$findingsSource = Join-Path $currentDir "findings.md"
if (-not (Test-Path $findingsSource)) {
    Write-Error "findings.md not found in current/"
    exit 1
}

# 計画ディレクトリを作成（存在しない場合）
if (-not (Test-Path $featureDir)) {
    New-Item -ItemType Directory -Path $featureDir -Force | Out-Null
    Write-Host "Created: .claude/plans/$FeatureName/"
}

# findings.md を 00-investigation.md としてコピー
$investigationDest = Join-Path $featureDir "00-investigation.md"
Copy-Item -Path $findingsSource -Destination $investigationDest -Force
Write-Host "Archived: .claude/plans/$FeatureName/00-investigation.md"

# task_plan.md から調査サマリーを抽出して追記（オプション）
$taskPlanSource = Join-Path $currentDir "task_plan.md"
if (Test-Path $taskPlanSource) {
    $taskPlanContent = Get-Content -Path $taskPlanSource -Raw -Encoding UTF8

    # 「推奨サブ計画案」セクションを抽出
    if ($taskPlanContent -match '## 推奨サブ計画案[\s\S]*?(?=##|$)') {
        $subplanSection = $Matches[0]

        # 00-investigation.md に追記
        $investigationContent = Get-Content -Path $investigationDest -Raw -Encoding UTF8
        $investigationContent += "`n`n---`n`n# 調査時の推奨サブ計画案`n`n$subplanSection"
        Set-Content -Path $investigationDest -Value $investigationContent -Encoding UTF8
        Write-Host "Appended: Recommended subplan section"
    }
}

Write-Host ""
Write-Host "Investigation archived for: $FeatureName" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Run: .\init-feature.ps1 $FeatureName [subplan-count]"
Write-Host "  2. Edit README.md and subplan files"
Write-Host "  3. Get user approval"
Write-Host "  4. Start implementation with /planning-with-files"
