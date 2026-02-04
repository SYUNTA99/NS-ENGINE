# Result型 - 大規模エンジン向けエラーハンドリングシステム

## 目的

大規模ゲームエンジンに必要な**包括的なエラーハンドリングシステム**を構築する。

nn SDKの設計を基盤としつつ、以下の実用的機能を追加：
- エラーコンテキスト（発生箇所、原因チェーン）
- エラーメッセージシステム
- エラー診断・統計機能
- モジュール別の豊富なエラー定義

## アーキテクチャ概要

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Result Type System                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                │
│  │    Core     │  │   Error     │  │  Context    │                │
│  │             │  │             │  │             │                │
│  │ ResultTraits│  │ErrorResult- │  │ResultContext│                │
│  │ ResultBase  │  │  Base       │  │SourceLoc   │                │
│  │ Result      │  │ErrorRange   │  │ErrorChain   │                │
│  │ ResultSucc- │  │ErrorCategory│  │             │                │
│  │   ess       │  │             │  │             │                │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘                │
│         │                │                │                        │
│         └────────────────┼────────────────┘                        │
│                          │                                         │
│  ┌───────────────────────┴───────────────────────┐                │
│  │                    Modules                     │                │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ │                │
│  │  │Common  │ │   FS   │ │Graphics│ │  ECS   │ │                │
│  │  └────────┘ └────────┘ └────────┘ └────────┘ │                │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ │                │
│  │  │  OS    │ │ Memory │ │ Audio  │ │ Input  │ │                │
│  │  └────────┘ └────────┘ └────────┘ └────────┘ │                │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ │                │
│  │  │Network │ │Resource│ │ Scene  │ │Physics │ │                │
│  │  └────────┘ └────────┘ └────────┘ └────────┘ │                │
│  └───────────────────────────────────────────────┘                │
│                          │                                         │
│  ┌───────────────────────┴───────────────────────┐                │
│  │                   Utility                      │                │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ │                │
│  │  │   Macros   │ │ Formatter  │ │ Registry   │ │                │
│  │  └────────────┘ └────────────┘ └────────────┘ │                │
│  └───────────────────────────────────────────────┘                │
│                          │                                         │
│  ┌───────────────────────┴───────────────────────┐                │
│  │                 Diagnostics                    │                │
│  │  ┌────────────────┐  ┌────────────────┐       │                │
│  │  │   Statistics   │  │    Logging     │       │                │
│  │  └────────────────┘  └────────────────┘       │                │
│  └───────────────────────────────────────────────┘                │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

## サブ計画

| # | 計画 | 状態 | 概要 |
|---|------|------|------|
| 1 | [Core: ResultTraits](01-result-traits.md) | pending | ビットレイアウト、値操作 |
| 2 | [Core: ResultBase・Result](02-result-base.md) | pending | CRTP、基本Result、ResultSuccess |
| 3 | [Error: ErrorResultBase・ErrorRange](03-error-result.md) | pending | エラー型定義、範囲マッチング |
| 4 | [Error: ErrorCategory・ErrorInfo](04-error-category.md) | pending | カテゴリ分類、詳細情報 |
| 5 | [Context: SourceLocation・ResultContext](05-context.md) | pending | 発生箇所、コンテキスト |
| 6 | [Context: ErrorChain](06-error-chain.md) | pending | エラー原因の連鎖追跡 |
| 7 | [Module: ModuleId・CommonResult](07-module-common.md) | pending | モジュールID、共通エラー |
| 8 | [Module: System Results](08-module-system.md) | pending | FS, OS, Memory, Network |
| 9 | [Module: Engine Results](09-module-engine.md) | pending | Graphics, ECS, Audio, Input, Resource, Scene, Physics |
| 10 | [Utility: Macros](10-macros.md) | pending | ヘルパーマクロ |
| 11 | [Utility: Formatter・Registry](11-formatter-registry.md) | pending | メッセージ、型登録 |
| 12 | [Diagnostics: Statistics・Logging](12-diagnostics.md) | pending | 統計、ログ統合 |
| 13 | [Integration: Tests](13-tests.md) | pending | 統合テスト |

## ファイル構成

```
Source/common/result/
├── Core/
│   ├── ResultTraits.h           # ビットレイアウト、値操作
│   ├── ResultBase.h             # CRTPベースクラス
│   ├── Result.h                 # Result、ResultSuccess
│   ├── Result.cpp               # OnUnhandledResult実装
│   └── InternalAccessor.h       # 内部アクセス
│
├── Error/
│   ├── ErrorResultBase.h        # エラー型ベーステンプレート
│   ├── ErrorRange.h             # 範囲マッチング
│   ├── ErrorCategory.h          # Transient/Permanent, Recoverable/Fatal
│   ├── ErrorInfo.h              # 詳細エラー情報構造体
│   └── ErrorDefines.h           # NS_DEFINE_ERROR_RANGE_RESULT等
│
├── Context/
│   ├── SourceLocation.h         # ファイル名、行番号、関数名
│   ├── ResultContext.h          # コンテキスト付きResult
│   └── ErrorChain.h             # エラー原因の連鎖
│
├── Module/
│   ├── ModuleId.h               # モジュールID定数
│   ├── CommonResult.h           # 共通エラー（50+種）
│   ├── FileSystemResult.h       # ファイルシステム（30+種）
│   ├── OsResult.h               # OS（20+種）
│   ├── MemoryResult.h           # メモリ管理（15+種）
│   ├── NetworkResult.h          # ネットワーク（25+種）
│   ├── GraphicsResult.h         # グラフィックス（40+種）
│   ├── EcsResult.h              # ECS（20+種）
│   ├── AudioResult.h            # オーディオ（15+種）
│   ├── InputResult.h            # 入力（10+種）
│   ├── ResourceResult.h         # リソース管理（20+種）
│   ├── SceneResult.h            # シーン管理（15+種）
│   └── PhysicsResult.h          # 物理（15+種）
│
├── Utility/
│   ├── ResultMacros.h           # ヘルパーマクロ
│   ├── ResultFormatter.h        # エラーメッセージフォーマット
│   └── ResultRegistry.h         # エラー型レジストリ
│
├── Diagnostics/
│   ├── ResultStatistics.h       # エラー統計収集
│   └── ResultLogging.h          # ログシステム統合
│
└── ResultAll.h                  # 統合ヘッダ
```

## 主要機能

### 1. 基本Result型（nn SDK互換）

```cpp
NS::Result result = SomeOperation();
if (result.IsFailure()) {
    int module = result.GetModule();
    int desc = result.GetDescription();
}
```

### 2. エラーカテゴリ

```cpp
// エラーの性質による分類
enum class ErrorCategory {
    Transient,    // 一時的（リトライ可能）
    Permanent,    // 永続的（リトライ不可）
    Recoverable,  // 回復可能
    Fatal,        // 致命的
};

if (GetErrorCategory(result) == ErrorCategory::Transient) {
    // リトライ処理
}
```

### 3. コンテキスト付きResult

```cpp
// エラー発生箇所を自動記録
NS_RETURN_IF_FAILED_CTX(LoadFile(path));

// コンテキスト情報を取得
auto ctx = GetResultContext(result);
LOG_ERROR("Error at {}:{} in {}", ctx.file, ctx.line, ctx.function);
```

### 4. エラーチェーン

```cpp
// 原因エラーを連鎖
NS::Result innerError = ReadFile(path);
if (innerError.IsFailure()) {
    return NS::MakeChainedResult(
        ResourceResult::ResultLoadFailed(),
        innerError,
        "Failed to load texture"
    );
}

// チェーンを辿る
for (auto& error : GetErrorChain(result)) {
    LOG_ERROR("  Caused by: {}", FormatResult(error));
}
```

### 5. エラーメッセージ

```cpp
// 人間が読めるメッセージを取得
std::string message = FormatResult(result);
// "FileSystem::PathNotFound (Module=2, Desc=1): 指定されたパスが見つかりません"

// 詳細情報
ErrorInfo info = GetErrorInfo(result);
LOG_ERROR("Error: {} (Retriable: {})", info.message, info.isRetriable);
```

### 6. 診断・統計

```cpp
// エラー統計を収集
ResultStatistics::RecordError(result);

// 頻出エラーを確認
auto topErrors = ResultStatistics::GetTopErrors(10);
for (auto& [result, count] : topErrors) {
    LOG_INFO("{}: {} occurrences", FormatResult(result), count);
}
```

## 設計決定

| 決定 | 理由 |
|------|------|
| nn SDK互換の32bitレイアウト | 実績ある設計、メモリ効率 |
| CRTPパターン | ゼロコスト多態性 |
| ErrorCategoryをReserved領域に格納 | constexpr対応、レジストリ不要 |
| SourceLocation (C++20) | 自動的な発生箇所記録 |
| ErrorChain | 根本原因の追跡 |
| ResultRegistry | 文字列変換、デバッグ支援 |
| ResultStatistics | 本番環境での問題分析 |
| モジュール別ファイル分割 | 依存関係の明確化、コンパイル時間短縮 |
| LRUベースのContext/Chain Storage | 固定サイズ配列より柔軟、寿命問題回避 |
| shared_mutex for Registry | 読み取り並行性、初期化後は読み取りのみ |

## ビットレイアウト詳細

```
32bit Result Value
┌───────────────────────────────────────────────────────────────┐
│ bit31-26 │ bit25-24   │ bit23-22     │ bit21-9     │ bit8-0  │
│ (6bit)   │ (2bit)     │ (2bit)       │ (13bit)     │ (9bit)  │
│ 将来拡張 │ Severity   │ Persistence  │ Description │ Module  │
│ 未使用   │ 0=Unknown  │ 0=Unknown    │ 詳細エラー  │ モジュ  │
│          │ 1=Recover  │ 1=Transient  │ 0-8191      │ ールID  │
│          │ 2=Fatal    │ 2=Permanent  │             │ 1-511   │
└───────────────────────────────────────────────────────────────┘
値 = 0: 成功
値 ≠ 0: 失敗
```

## 検証方法

1. ビルド成功（Debug/Release）
2. `std::is_trivially_copyable<Result>` が true
3. 全モジュールのエラー定義が正しく動作
4. コンテキスト・チェーン機能の動作確認
5. フォーマッタ・レジストリの動作確認
6. 診断機能の動作確認
7. パフォーマンステスト（オーバーヘッド計測）
