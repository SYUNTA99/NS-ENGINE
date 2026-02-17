<#
.SYNOPSIS
    サブ計画の規模・数を検証する

.PARAMETER FeatureName
    機能名

.EXAMPLE
    .\verify-subplans.ps1 rhi
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$FeatureName
)

$ErrorActionPreference = "Stop"

$projectRoot = (Get-Item "$PSScriptRoot/../../../..").FullName
$featureDir = Join-Path $projectRoot ".claude/plans/$FeatureName"
$readmePath = Join-Path $featureDir "README.md"

if (-not (Test-Path $readmePath)) {
    Write-Error "Feature not found: $FeatureName"
    exit 1
}

# --- ファイル分類 ---
$allFiles = Get-ChildItem -Path $featureDir -Filter "*.md" |
    Where-Object { $_.Name -match '^\d{2}-' } |
    Sort-Object Name

# 00-* は調査ファイル、01-* 以降がサブ計画
$investigationFiles = $allFiles | Where-Object { $_.Name -match '^00-' }
$subplanFiles = $allFiles | Where-Object { $_.Name -notmatch '^00-' }

$subplanCount = $subplanFiles.Count

Write-Host ""
Write-Host "=== Subplan Verification: $FeatureName ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Investigation files: $($investigationFiles.Count)"
Write-Host "Subplan count:       $subplanCount"
Write-Host ""

# --- 各サブ計画の解析 ---
$issues = @()
$moduleSet = @{}
$totalFilePaths = 0

foreach ($file in $subplanFiles) {
    $content = Get-Content -Path $file.FullName -Raw -Encoding UTF8
    $name = $file.Name

    # TODO 数カウント (未完了 + 完了)
    $todoMatches = [regex]::Matches($content, '- \[ \]')
    $todoCount = $todoMatches.Count
    $doneMatches = [regex]::Matches($content, '- \[x\]', 'IgnoreCase')
    $doneCount = $doneMatches.Count
    $totalTodos = $todoCount + $doneCount

    # ファイルパス検出 (Source/... or .h/.cpp/.hlsl 等)
    $filePathMatches = [regex]::Matches($content, '(?m)(?:Source/[\w/]+\.\w+|[\w/]+\.(?:h|cpp|hlsl|ixx))')
    $filePathCount = ($filePathMatches | ForEach-Object { $_.Value } | Sort-Object -Unique).Count
    $totalFilePaths += $filePathCount

    # モジュール検出 (Source/Engine/<Module>/ or Source/Game/<Module>/)
    $moduleMatches = [regex]::Matches($content, '(?i)Source/(?:Engine|Game)/(\w+)/')
    foreach ($m in $moduleMatches) {
        $moduleName = $m.Groups[1].Value
        if (-not $moduleSet.ContainsKey($moduleName)) {
            $moduleSet[$moduleName] = @()
        }
        if ($moduleSet[$moduleName] -notcontains $name) {
            $moduleSet[$moduleName] += $name
        }
    }

    # --- 上限チェック ---
    $status = "OK"
    $fileIssues = @()

    if ($totalTodos -gt 5) {
        $fileIssues += "TODO $totalTodos > 5"
    }
    if ($filePathCount -gt 8) {
        $fileIssues += "files mentioned $filePathCount > 8"
    }

    if ($fileIssues.Count -gt 0) {
        $status = "WARN"
        $color = "Yellow"
        foreach ($fi in $fileIssues) {
            $issues += "[UPPER_LIMIT] $name : $fi"
        }
    } else {
        $color = "Green"
    }

    Write-Host "  $name : TODO=$totalTodos, files=$filePathCount [$status]" -ForegroundColor $color
}

# --- 最低サブ計画数チェック ---
Write-Host ""
Write-Host "--- Scale Analysis ---" -ForegroundColor Cyan

$minFromFiles = 1
if ($totalFilePaths -ge 60) { $minFromFiles = 8 }
elseif ($totalFilePaths -ge 30) { $minFromFiles = 5 }
elseif ($totalFilePaths -ge 15) { $minFromFiles = 3 }
elseif ($totalFilePaths -ge 6) { $minFromFiles = 2 }

$moduleCount = $moduleSet.Count
$minFromModules = 1
if ($moduleCount -ge 4) { $minFromModules = $moduleCount + 1 }
elseif ($moduleCount -ge 2) { $minFromModules = $moduleCount }

$requiredMin = [math]::Max($minFromFiles, $minFromModules)

Write-Host "  Total file references:     $totalFilePaths  -> min subplans: $minFromFiles"
Write-Host "  Modules touched:           $moduleCount  -> min subplans: $minFromModules"
Write-Host "  Required minimum:          $requiredMin"
Write-Host "  Actual subplan count:      $subplanCount"

if ($subplanCount -lt $requiredMin) {
    $issues += "[MIN_COUNT] Subplan count $subplanCount < required minimum $requiredMin"
    Write-Host "  Result: INSUFFICIENT" -ForegroundColor Red
} else {
    Write-Host "  Result: OK" -ForegroundColor Green
}

# --- モジュール境界チェック ---
Write-Host ""
Write-Host "--- Module Boundary Check ---" -ForegroundColor Cyan

foreach ($mod in $moduleSet.Keys) {
    $files = $moduleSet[$mod]
    $fileList = $files -join ', '
    if ($fileList.Length -gt 120) {
        $fileList = "$($files.Count) subplans"
    }
    Write-Host "  $mod : $fileList"
}

# --- サマリ ---
Write-Host ""
if ($issues.Count -eq 0) {
    Write-Host "=== PASS: No issues found ===" -ForegroundColor Green
} else {
    Write-Host "=== ISSUES FOUND: $($issues.Count) ===" -ForegroundColor Yellow
    $shown = 0
    foreach ($issue in $issues) {
        Write-Host "  ! $issue" -ForegroundColor Yellow
        $shown++
        if ($shown -ge 20) {
            $remaining = $issues.Count - $shown
            if ($remaining -gt 0) {
                Write-Host "  ... and $remaining more" -ForegroundColor Yellow
            }
            break
        }
    }
}

Write-Host ""
