---
description: "依存関係のセキュリティ脆弱性を監査"
---

# Dependency Security Audit

C++プロジェクトの外部依存関係に対するセキュリティ監査を実行します。

## Step 1: 現在の依存関係を確認

以下のソースから依存ライブラリとバージョンを特定:

1. **external/** ディレクトリ:
   - DirectXTex/DirectXTK (Microsoft)
   - Google Test (`external/googletest/`)

2. **premake5.lua**:
   - Assimp バージョン情報

3. **packages/packages.config** (NuGet):
   - tinygltf

各ライブラリのバージョンを正確に特定してください。

## Step 2: 脆弱性チェック

各ライブラリについて WebSearch ツールで検索:

1. **CVE検索**: `{library_name} {version} CVE vulnerability 2024 2025`
2. **GitHub Advisory**: `{library_name} security advisory github`
3. **最新バージョン確認**: `{library_name} latest release version`

検索対象:
- DirectXTex CVE vulnerability
- DirectXTK security advisory
- Assimp 6.0.2 CVE vulnerability
- tinygltf security vulnerability
- googletest security advisory

## Step 3: 結果を分析

各脆弱性について以下を評価:
- **CVE番号**: 識別子
- **深刻度**: Critical / High / Medium / Low
- **影響**: このプロジェクトへの影響有無
- **修正版**: パッチが提供されているか
- **回避策**: アップグレード以外の緩和策

## Step 4: レポート出力

### 現在の依存関係

| ライブラリ | 現在バージョン | 最新バージョン | 状態 |
|-----------|---------------|---------------|------|
| DirectXTex | x.x | x.x | 最新/更新推奨 |
| DirectXTK | x.x | x.x | 最新/更新推奨 |
| Assimp | 6.0.2 | x.x | 最新/更新推奨 |
| tinygltf | x.x | x.x | 最新/更新推奨 |
| Google Test | 1.15.2 | x.x | 最新/更新推奨 |

### 検出された脆弱性

| ライブラリ | CVE | 深刻度 | 影響 | 修正版 | 推奨対応 |
|-----------|-----|--------|------|--------|---------|

### 推奨アクション

優先度順にアップグレード手順を提示:

1. **Critical/High**: 即座に対応が必要
2. **Medium**: 計画的にアップグレード
3. **Low**: 次回メンテナンス時に対応

各アップグレードについて:
- 破壊的変更の有無
- アップグレード手順
- テスト確認項目
