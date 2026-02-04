<#
.SYNOPSIS
    次のサブ計画を特定する

.PARAMETER FeatureName
    機能名

.EXAMPLE
    .\next-subplan.ps1 rhi
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
    Write-Host "No subplans found in README.md" -ForegroundColor Yellow
    exit 1
}

$nextSubplan = $null

foreach ($match in $matches) {
    $status = $match.Groups[4].Value
    if ($status -eq "pending") {
        $nextSubplan = @{
            Num = $match.Groups[1].Value
            Name = $match.Groups[2].Value
            File = $match.Groups[3].Value
        }
        break
    }
}

if ($null -eq $nextSubplan) {
    Write-Host "All subplans complete!" -ForegroundColor Green
    exit 0
}

$subplanPath = ".claude/plans/$FeatureName/$($nextSubplan.File)"
Write-Host "Next subplan: $subplanPath" -ForegroundColor Cyan
