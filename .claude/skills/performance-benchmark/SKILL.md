---
description: "ECS/GPU パフォーマンス測定"
allowed-tools:
  - "Bash(build/bin/*:*)"
  - "Bash(tools\\@build.cmd:*)"
  - "Bash(tools\\@run_tests.cmd:*)"
---

# Performance Benchmark

ECSとGPUのパフォーマンスベンチマークを実行し、結果を分析します。

## 使用方法

`/performance-benchmark [filter]`

- 引数なし: 全Performanceテストを実行
- フィルタ指定: 特定のベンチマークのみ実行

例:
- `/performance-benchmark` → 全ベンチマーク
- `/performance-benchmark ECS` → ECS関連のみ
- `/performance-benchmark ForEach` → ForEachベンチマークのみ

## 実行手順

### 1. Release ビルド

```bash
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'tools\@build.cmd Release' 2>&1"
```

### 2. ベンチマーク実行

引数なしの場合:
```bash
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'build\bin\Release-windows-x86_64\tests\tests.exe --gtest_filter=*Performance* --gtest_color=yes' 2>&1"
```

フィルタ指定の場合:
```bash
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'build\bin\Release-windows-x86_64\tests\tests.exe --gtest_filter=*Performance*$ARGUMENTS* --gtest_color=yes' 2>&1"
```

timeout: 600000ms（10分）

### 3. 結果分析

出力から以下を抽出してレポート:

#### パフォーマンス指標

| 指標 | 説明 |
|------|------|
| 実行時間 | テスト全体の経過時間 |
| エンティティ/秒 | ForEachスループット |
| メモリ使用量 | Chunk充填率 |
| キャッシュミス | （プロファイラ併用時） |

#### ボトルネック検出

- **O(n²)パターン**: 2倍のエンティティで4倍以上の時間
- **メモリ帯域制限**: 大量データで急激な性能低下
- **ロック競合**: マルチスレッドで効率悪化

## レポートフォーマット

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 PERFORMANCE BENCHMARK RESULTS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔧 Build: Release (LTO enabled)
⏱️ Total Time: X.XXs

┌─────────────────────────────────────────────────┐
│ Test Name                      │ Time    │ Status│
├─────────────────────────────────────────────────┤
│ ECSPerformance.ForEach_10k    │ 0.5ms   │ ✓     │
│ ECSPerformance.ForEach_100k   │ 4.2ms   │ ✓     │
│ ECSPerformance.CreateDestroy  │ 12.3ms  │ ✓     │
└─────────────────────────────────────────────────┘

📈 ANALYSIS
───────────────────────────────────────────────────

ForEach Performance:
  - 10k entities: ~0.05μs/entity
  - 100k entities: ~0.042μs/entity (cache-friendly ✓)
  - Scaling: Linear O(n) ✓

Memory Efficiency:
  - Chunk utilization: ~95%
  - Cache line alignment: 64B ✓

⚠️ RECOMMENDATIONS
───────────────────────────────────────────────────

1. [最適化提案があれば記載]
2. [ボトルネックがあれば記載]

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## パフォーマンス目標（CLAUDE.md参照）

| アクセス | 目標 |
|----------|------|
| OOP（キャッシュ）1回目 | 20ns |
| OOP（キャッシュ）2回目以降 | 1ns |
| ECS ForEach | 0.1ns/entity |

## 注意事項

- **必ずReleaseビルド**で実行（Debugは10倍以上遅い）
- **LTO有効**で最適化されたバイナリを使用
- **バックグラウンドプロセス**を最小限に
- **複数回実行**して平均を取ることを推奨
