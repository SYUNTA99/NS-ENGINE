# Claude Code コマンド一覧

## ビルド

| コマンド | 説明 |
|----------|------|
| [`/make`](skills/make/SKILL.md) | プロジェクトをフルリビルド（クリーン→生成→ビルド） |
| [`/build-debug`](skills/build-debug/SKILL.md) | Debug ビルド |
| [`/build-release`](skills/build-release/SKILL.md) | Release ビルド（最適化有効） |
| [`/build-burst`](skills/build-burst/SKILL.md) | Burst ビルド（最大最適化、LTO） |
| [`/regen`](skills/regen/SKILL.md) | Visual Studio ソリューションを再生成 |
| [`/clean`](skills/clean/SKILL.md) | ビルド成果物をクリーンアップ |
| [`/test`](skills/test/SKILL.md) | テストを実行 |

## Git 操作

| コマンド | 説明 |
|----------|------|
| [`/git-status`](skills/git-status/SKILL.md) | 作業ツリーの状態を表示 |
| [`/git-diff`](skills/git-diff/SKILL.md) | 変更差分を表示 |
| [`/git-log`](skills/git-log/SKILL.md) | コミット履歴を簡潔に表示 |
| [`/git-commit`](skills/git-commit/SKILL.md) | 規約に沿ったコミットメッセージで commit |
| [`/git-push`](skills/git-push/SKILL.md) | 現在のブランチを push |
| [`/git-pull`](skills/git-pull/SKILL.md) | 現在のブランチを pull |
| [`/git-fetch`](skills/git-fetch/SKILL.md) | リモートの最新情報を取得 |
| [`/git-branch`](skills/git-branch/SKILL.md) | ブランチ操作（l=一覧, c=作成, d=削除） |
| [`/git-checkout`](skills/git-checkout/SKILL.md) | ブランチを切り替え |
| [`/git-stash`](skills/git-stash/SKILL.md) | 変更を一時退避 |
| [`/git-pr`](skills/git-pr/SKILL.md) | PR 作成ワークフロー |

## コード生成・分析

| コマンド | 説明 |
|----------|------|
| [`/component-generator`](skills/component-generator/SKILL.md) | ECS ComponentData + System テンプレート生成 |
| [`/explain-code`](skills/explain-code/SKILL.md) | コードをアナロジーと ASCII 図で説明 |
| [`/memory-layout-analyzer`](skills/memory-layout-analyzer/SKILL.md) | Chunk メモリレイアウトを可視化 |
| [`/dependency-graph`](skills/dependency-graph/SKILL.md) | モジュール依存関係の可視化 |
| [`/architecture`](skills/architecture/SKILL.md) | アーキテクチャ図生成（ASCII/Mermaid） |

## 品質・テスト

| コマンド | 説明 |
|----------|------|
| [`/code-review`](skills/code-review/SKILL.md) | コードレビュー（セキュリティ・品質・パフォーマンス） |
| [`/lint`](skills/lint/SKILL.md) | clang-tidy で静的解析・自動修正 |
| [`/tdd`](skills/tdd/SKILL.md) | テスト駆動開発ガイド（Red-Green-Refactor） |
| [`/performance-benchmark`](skills/performance-benchmark/SKILL.md) | ECS/GPU パフォーマンス測定 |
| [`/shader-compiler-check`](skills/shader-compiler-check/SKILL.md) | HLSL コンパイル検証 |

## 修正・デバッグ

| コマンド | 説明 |
|----------|------|
| [`/smart-fix`](skills/smart-fix/SKILL.md) | インテリジェントなバグ修正（根本原因分析） |
| [`/fix-github-issue`](skills/fix-github-issue/SKILL.md) | GitHub Issue を自動修正 |
| [`/fix-pipeline`](skills/fix-pipeline/SKILL.md) | CI 失敗を自動修正 |

## セキュリティ

| コマンド | 説明 |
|----------|------|
| [`/security-scan`](skills/security-scan/SKILL.md) | セキュリティ脆弱性スキャン |
| [`/dependency-audit`](skills/dependency-audit/SKILL.md) | 依存関係の脆弱性を監査 |

## 計画・レビュー

| コマンド | 説明 |
|----------|------|
| [`/planning-with-files`](skills/planning-with-files/SKILL.md) | ファイルベースの計画管理（task_plan.md, findings.md, progress.md） |
| [`/plan-doc-checker`](skills/plan-doc-checker/SKILL.md) | 計画とドキュメントの整合性チェック（齟齬・完全性・矛盾） |
| [`/plan-doc-review`](skills/plan-doc-review/SKILL.md) | ドキュメント参照型計画の厳格レビュー |
| [`/plan-accuracy-predictor`](skills/plan-accuracy-predictor/SKILL.md) | 計画の実装精度を予測・評価 |

## ドキュメント

| コマンド | 説明 |
|----------|------|
| [`/onboard`](skills/onboard/SKILL.md) | プロジェクト理解・オンボーディング |
| [`/prd-generator`](skills/prd-generator/SKILL.md) | 要件定義書（PRD）を自動生成 |
