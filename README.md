# HEW2026

チーム「Nihonium113」の制作物です。

学校主催のゲーム開発コンテスト「HEW」に向けて制作中のゲームプロジェクトです。

## 環境

- OS: Windows 10 以上
- IDE: Visual Studio 2022
- 言語: C++20
- グラフィックス: DirectX 11

## ビルド方法

### クローン

```bash
git clone --recursive https://github.com/HEW2026-Nihonium113/HEW2026.git
cd HEW2026
```

### ビルド・実行

```bash
# プロジェクト生成 + ビルド
@make_project.cmd

# 実行
bin\Debug-windows-x64\game\game.exe
```

### その他のコマンド

| コマンド | 説明 |
|---------|------|
| `@make_project.cmd` | プロジェクト生成 + ビルド |
| `@open_project.cmd` | Visual Studioで開く |
| `@cleanup.cmd` | ビルド成果物を削除 |
| `build_debug.cmd` | Debugビルドのみ |
| `build_release.cmd` | Releaseビルドのみ |

## ディレクトリ構成

```
source/
├── dx11/     # DirectX 11 ラッパーライブラリ
├── engine/   # ゲームエンジン層
└── game/     # ゲーム本体
```

## 開発の流れ

**masterへの直接pushは禁止されています。** 必ずPR（Pull Request）を作成してください。

### 1. ブランチを作成

```bash
git checkout master
git pull
git checkout -b feature/作業内容
```

ブランチ名の例：
- `feature/player-movement` - 新機能
- `fix/collision-bug` - バグ修正
- `refactor/shader-manager` - リファクタリング

### 2. 作業してコミット

```bash
git add .
git commit -m "変更内容を日本語で書く"
```

### 3. pushしてPR作成

```bash
git push -u origin feature/作業内容
```

pushしたらGitHubでPRを作成：
1. [リポジトリ](https://github.com/HEW2026-Nihonium113/HEW2026) にアクセス
2. 「Compare & pull request」ボタンをクリック
3. タイトルと説明を書いて「Create pull request」

### 4. レビュー → マージ

- **CodeRabbit**が自動でコードレビューします
- 問題なければマージしてください
- マージ後、ローカルのmasterを更新：

```bash
git checkout master
git pull
```

## チーム

Nihonium113
