<#
.SYNOPSIS
    計画ディレクトリを初期化する

.PARAMETER FeatureName
    機能名（ディレクトリ名として使用）

.PARAMETER SubplanCount
    作成するサブ計画ファイルの数（省略可）

.EXAMPLE
    .\init-feature.ps1 rhi
    .\init-feature.ps1 rhi 3
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$FeatureName,

    [Parameter(Position=1)]
    [int]$SubplanCount = 0
)

$ErrorActionPreference = "Stop"

$projectRoot = (Get-Item "$PSScriptRoot/../../../..").FullName
$plansDir = Join-Path $projectRoot ".claude/plans"
$featureDir = Join-Path $plansDir $FeatureName

if (Test-Path $featureDir) {
    Write-Error "Directory already exists: $featureDir"
    exit 1
}

New-Item -ItemType Directory -Path $featureDir -Force | Out-Null
Write-Host "Created: .claude/plans/$FeatureName/"

$readmeContent = @"
# $FeatureName

## 目的

<!-- 何を実現するか -->

## 影響範囲

<!-- どのモジュール・ファイルに影響するか -->

## サブ計画

| # | 計画 | 状態 |
|---|------|------|

## 検証方法

<!-- 全体が完了したときの検証方法 -->

## 設計決定

<!-- 重要な設計判断とその理由 -->
"@

$readmePath = Join-Path $featureDir "README.md"
Set-Content -Path $readmePath -Value $readmeContent -Encoding UTF8
Write-Host "Created: .claude/plans/$FeatureName/README.md"

if ($SubplanCount -gt 0) {
    $tableRows = @()
    for ($i = 1; $i -le $SubplanCount; $i++) {
        $paddedNum = "{0:D2}" -f $i
        $filename = "$paddedNum-subplan.md"
        $filepath = Join-Path $featureDir $filename

        $subplanContent = @"
# サブ計画 $i

## 目的

<!-- このサブ計画で達成すること -->

## 変更対象ファイル

<!-- 対象ファイル一覧 -->

## TODO

- [ ]

## 検証方法

<!-- このサブ計画の検証方法 -->

## 知見

<!-- 実装中に得られた知見を記録 -->
"@
        Set-Content -Path $filepath -Value $subplanContent -Encoding UTF8
        Write-Host "Created: .claude/plans/$FeatureName/$filename"

        $tableRows += "| $i | [サブ計画$i]($filename) | pending |"
    }

    $readmeWithTable = $readmeContent -replace "\| # \| 計画 \| 状態 \|\r?\n\|---\|------\|------\|", (
        "| # | 計画 | 状態 |`n|---|------|------|`n" + ($tableRows -join "`n")
    )
    Set-Content -Path $readmePath -Value $readmeWithTable -Encoding UTF8
}

Write-Host ""
Write-Host "Feature planning initialized: $FeatureName" -ForegroundColor Green
