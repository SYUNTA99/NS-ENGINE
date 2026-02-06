# 01-01: ディレクトリ構造

## 目的

HALモジュールの3層ディレクトリ構造を作成する。

## 参考

[Platform_Abstraction_Layer_Part1.md](docs/Platform_Abstraction_Layer_Part1.md) - セクション2「全体アーキテクチャ」

## 依存（commonモジュール）

（なし - ディレクトリ作成のみ）

## 依存（HAL）

（なし - 最初のサブ計画）

## 変更対象

新規作成:
- `source/engine/hal/`
- `source/engine/hal/Public/HAL/`
- `source/engine/hal/Public/GenericPlatform/`
- `source/engine/hal/Public/Windows/`
- `source/engine/hal/Private/`
- `source/engine/hal/Private/Windows/`
- `source/engine/hal/Private/HAL/`

## TODO

- [ ] `source/engine/hal/` ルートディレクトリ作成
- [ ] `Public/HAL/` 作成（統一インターフェース層）
- [ ] `Public/GenericPlatform/` 作成（プラットフォーム非依存の汎用実装）
- [ ] `Public/Windows/` 作成（Windows固有ヘッダ）
- [ ] `Private/Windows/` 作成（Windows実装cpp）
- [ ] `Private/HAL/` 作成（プラットフォーム非依存実装cpp）

## 検証

```
source/engine/hal/
├── Public/
│   ├── HAL/              # #include "HAL/Platform.h" でアクセス
│   ├── GenericPlatform/  # #include "GenericPlatform/..." でアクセス
│   └── Windows/          # #include "Windows/..." でアクセス
└── Private/
    ├── HAL/              # 共通実装
    └── Windows/          # Windows固有実装
```

## 備考

- 他プラットフォーム（Mac/, Linux/）は後で追加可能
- Public/ 配下のみが他モジュールからインクルード可能
- Private/ 配下はHALモジュール内部のみ

## 次のサブ計画

→ 01-02: プラットフォームマクロ（Platform.h作成）
