<#
.SYNOPSIS
    機能全体の進捗を確認する

.PARAMETER FeatureName
    機能名

.EXAMPLE
    .\feature-status.ps1 rhi
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

$content = Get-Content -Path $readmePath -Raw

$pattern = '\|\s*(\d+)\s*\|\s*\[([^\]]+)\]\(([^)]+)\)\s*\|\s*(pending|complete)\s*\|'
$matches = [regex]::Matches($content, $pattern)

if ($matches.Count -eq 0) {
    Write-Host "=== Feature Status: $FeatureName ===" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "No subplans found in README.md" -ForegroundColor Yellow
    Write-Host "Add subplans in the table format:"
    Write-Host "| # | 計画 | 状態 |"
    Write-Host "|---|------|------|"
    Write-Host "| 1 | [サブ計画1](01-xxx.md) | pending |"
    exit 0
}

$total = $matches.Count
$complete = 0
$pending = 0
$subplans = @()

foreach ($match in $matches) {
    $num = $match.Groups[1].Value
    $name = $match.Groups[2].Value
    $file = $match.Groups[3].Value
    $status = $match.Groups[4].Value

    if ($status -eq "complete") {
        $complete++
        $mark = "[X]"
        $color = "Green"
    } else {
        $pending++
        $mark = "[ ]"
        $color = "White"
    }

    $subplans += @{
        Num = $num
        Name = $name
        File = $file
        Status = $status
        Mark = $mark
        Color = $color
    }
}

$progress = [math]::Round(($complete / $total) * 100)

Write-Host ""
Write-Host "=== Feature Status: $FeatureName ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Total subplans:  $total"
Write-Host "Complete:        $complete" -ForegroundColor Green
Write-Host "Pending:         $pending" -ForegroundColor Yellow
Write-Host "Progress:        $progress%"
Write-Host ""

foreach ($sp in $subplans) {
    Write-Host "$($sp.Mark) $($sp.File)" -ForegroundColor $sp.Color
}

Write-Host ""
