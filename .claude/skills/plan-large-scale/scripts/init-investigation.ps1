<#
.SYNOPSIS
    調査セッションを初期化する

.PARAMETER FeatureName
    調査対象の機能名

.EXAMPLE
    .\init-investigation.ps1 rhi
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$FeatureName
)

$ErrorActionPreference = "Stop"

$projectRoot = (Get-Item "$PSScriptRoot/../../../..").FullName
$currentDir = Join-Path $projectRoot ".claude/plans/current"
$templateDir = Join-Path $projectRoot ".claude/skills/planning-with-files/templates"

# current/ が存在する場合は警告
if (Test-Path $currentDir) {
    Write-Warning "current/ already exists. Contents will be overwritten."
    Write-Host "Existing files:"
    Get-ChildItem $currentDir | ForEach-Object { Write-Host "  - $($_.Name)" }
    Write-Host ""
}

# current/ を作成
New-Item -ItemType Directory -Path $currentDir -Force | Out-Null

# investigation_plan.md を task_plan.md としてコピー
$investigationTemplate = Join-Path $templateDir "investigation_plan.md"
$taskPlanPath = Join-Path $currentDir "task_plan.md"

if (Test-Path $investigationTemplate) {
    $content = Get-Content -Path $investigationTemplate -Raw -Encoding UTF8
    # Target Feature を置換
    $content = $content -replace '\[調査対象の機能名\]', $FeatureName
    $content = $content -replace '\[Feature Name\]', $FeatureName
    Set-Content -Path $taskPlanPath -Value $content -Encoding UTF8
    Write-Host "Created: .claude/plans/current/task_plan.md (Mode: investigation)"
} else {
    Write-Error "Template not found: $investigationTemplate"
    exit 1
}

# findings.md を作成
$findingsTemplate = Join-Path $templateDir "findings.md"
$findingsPath = Join-Path $currentDir "findings.md"

if (Test-Path $findingsTemplate) {
    Copy-Item -Path $findingsTemplate -Destination $findingsPath -Force
    Write-Host "Created: .claude/plans/current/findings.md"
} else {
    # テンプレートがない場合は基本的な内容で作成
    $findingsContent = @"
# Findings & Decisions

## Requirements
- $FeatureName の調査

## Research Findings
-

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
|          |           |

## Resources
-
"@
    Set-Content -Path $findingsPath -Value $findingsContent -Encoding UTF8
    Write-Host "Created: .claude/plans/current/findings.md"
}

# progress.md を作成
$progressTemplate = Join-Path $templateDir "progress.md"
$progressPath = Join-Path $currentDir "progress.md"

if (Test-Path $progressTemplate) {
    $content = Get-Content -Path $progressTemplate -Raw -Encoding UTF8
    # セッション日付を今日に
    $today = Get-Date -Format "yyyy-MM-dd"
    $content = $content -replace '\[DATE\]', $today
    Set-Content -Path $progressPath -Value $content -Encoding UTF8
    Write-Host "Created: .claude/plans/current/progress.md"
} else {
    $today = Get-Date -Format "yyyy-MM-dd"
    $progressContent = @"
# Progress Log

## Session: $today

### Phase 1: 既存コードの理解
- **Status:** in_progress
- **Started:** $today
- Actions taken:
  -
- Files created/modified:
  -

## 5-Question Reboot Check
| Question | Answer |
|----------|--------|
| Where am I? | Phase 1: 既存コードの理解 |
| Where am I going? | Phase 2-4: 要件明確化・影響範囲・報告 |
| What's the goal? | $FeatureName の調査 |
| What have I learned? | See findings.md |
| What have I done? | See above |
"@
    Set-Content -Path $progressPath -Value $progressContent -Encoding UTF8
    Write-Host "Created: .claude/plans/current/progress.md"
}

Write-Host ""
Write-Host "Investigation session initialized for: $FeatureName" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Explore codebase and record findings in findings.md"
Write-Host "  2. Update task_plan.md as you progress"
Write-Host "  3. When done, run: .\archive-investigation.ps1 $FeatureName"
