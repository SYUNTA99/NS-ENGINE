# HAL実装計画

## 目的

NS-ENGINEにHALを実装。既存コード変更なし、新規追加のみ。

## 影響範囲

- `source/engine/hal/` - HALモジュール（新規）
- `premake5.lua` - HALプロジェクト追加

## commonモジュール依存

HALは `source/common/` の既存マクロ・型を基盤として使用する：

| common | HALエイリアス | 用途 |
|--------|--------------|------|
| `NS_PLATFORM_WINDOWS` | `PLATFORM_WINDOWS` | プラットフォーム識別 |
| `NS_COMPILER_MSVC` | `PLATFORM_COMPILER_MSVC` | コンパイラ識別 |
| `NS_FORCEINLINE` | `FORCEINLINE` | インライン強制 |
| `NS_NOINLINE` | `FORCENOINLINE` | インライン禁止 |
| `NS_EXPORT` | `DLLEXPORT` | DLLエクスポート |
| `NS_IMPORT` | `DLLIMPORT` | DLLインポート |
| `NS_CDECL` | `CDECL` | 呼び出し規約 |
| `NS_STDCALL` | `STDCALL` | 呼び出し規約 |
| `NS_RESTRICT` | `RESTRICT` | 制限付きポインタ |
| `NS_MACRO_STRINGIZE` | `NS_STRINGIFY` | 文字列化 |
| `NS_MACRO_CONCATENATE` | `NS_CONCAT` | トークン連結 |
| `uint8`〜`uint64` | （直接使用） | 基本整数型 |
| `int8`〜`int64` | （直接使用） | 基本整数型 |
| `size` | `SIZE_T` | サイズ型 |
| `ptrdiff` | `SSIZE_T` | 差分型 |

HAL固有の追加定義：
- 文字型: `TCHAR`, `ANSICHAR`, `WIDECHAR`
- ポインタ型: `UPTRINT`, `PTRINT`
- プラットフォームグループ: `PLATFORM_DESKTOP`, `PLATFORM_UNIX`
- CPUファミリー: `PLATFORM_CPU_X86_FAMILY`, `PLATFORM_CPU_ARM_FAMILY`

## 実装順序（依存関係）

```
Phase 1: 基盤（順序厳守、直列実装）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  [common/utility/macros.h, types.h] ← 既存
    ↓
  01-01 ディレクトリ構造
    ↓
  01-02 プラットフォームマクロ (commonのエイリアス)
    ↓
  01-03 COMPILED_PLATFORM_HEADER
    ↓
  01-04 プラットフォーム型 (HAL固有型追加)
    ↓
  02-01 機能フラグマクロ
    ↓
  02-02 コンパイラマクロ
    ↓
  02-03 関数呼び出し規約 → PlatformMacros.h 完成
    ↓
  04-01 premakeビルド設定 → ビルド可能状態

Phase 2A: コアコンポーネント（並列実装可能）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  ┌─────────────┬─────────────┬─────────────┐
  │ 03-01       │ 03-02       │ 03-03       │
  │ Memory      │ Atomics     │ Time        │
  └─────────────┴─────────────┴─────────────┘
  ┌─────────────┬─────────────┐
  │ 03-04       │ 03-05       │
  │ Process     │ File        │
  └─────────────┴─────────────┘

Phase 2B: スレッディング（並列実装可能）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  ┌─────────────┬─────────────┐
  │ 05-01       │ 05-02       │
  │ Affinity    │ TLS         │
  └─────────────┴─────────────┘
  ┌─────────────┬─────────────┐
  │ 05-03       │ 05-04       │
  │ StackWalk   │ CrashCtx    │
  └─────────────┴─────────────┘

Phase 3: メモリシステム（直列実装、依存関係あり）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  06-01a Malloc基底クラス
    ↓
  06-01b GMalloc グローバル
    ↓
  06-02a MallocAnsiクラス定義
    ↓
  06-02b MallocAnsi実装
    ↓
  06-03a Memory静的API
    ↓
  06-03b Memory API実装
    ↓
  07-01a LLMタグ列挙
    ↓
  07-01b LLMTrackerクラス
    ↓
  07-02a LLMScopeクラス
    ↓
  07-02b LLMScopeマクロ

Phase 4: ユーティリティ（並列実装可能）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  ┌─────────────┬─────────────┬─────────────┐
  │ 08-01a/b    │ 08-02a/b    │ 08-03a/b    │
  │ OutputDev   │ NamedEvent  │ ConsoleVar  │
  └─────────────┴─────────────┴─────────────┘
  ┌─────────────┬─────────────┬─────────────┐
  │ 09-01a/b    │ 09-02a/b    │ 10-01a/b    │
  │ CPUFeatures │ WinSpecific │ StringUtils │
  └─────────────┴─────────────┴─────────────┘
  ┌─────────────┬─────────────┬─────────────┐
  │ 10-02a      │ 10-03a/b    │ 10-04       │
  │ Alignment   │ Hash        │ StackAlloc  │
  └─────────────┴─────────────┴─────────────┘
```

## スコープ外

以下は本計画ではスコープ外（将来計画）：

### メモリアロケータ
- MallocBinned / Binned2 / Binned3（高性能ビンアロケータ）
- MallocTBB / Jemalloc / Mimalloc（サードパーティアロケータ）
- MallocStomp / MallocDebug（デバッグ専用アロケータ）
- アロケータ選択ロジック（コマンドライン引数による切り替え）

### LLM出力機構
- LLMCsvWriter（CSV出力）
- LLMTraceWriter（トレース出力）
- Unreal Insights統合
- 外部プロファイラ連携

### 非同期システム
- 非同期タスクシステム（Part10 セクション10〜14）
- スレッドプール/タスクグラフ（既存JobSystemと統合予定）

### その他
- 国際化システム
- ネットワーク抽象化レイヤー

## 並列実装ガイドライン

### 同一Phaseで並列可能な条件
1. 互いに依存関係がない
2. 同一ファイルを変更しない
3. Phase 1が完了済み

### 並列実装時の注意
- Phase 2A/2B/4は並列実装可能
- Phase 3は依存関係があるため直列必須
- Private/ 内の Windows/ サブディレクトリは複数作業者が同時作成可能

## テスト戦略

### テストファイル配置

```
tests/engine/hal/
├── MemoryTest.cpp           # 03-01, 06-xx テスト
├── AtomicsTest.cpp          # 03-02 テスト
├── TimeTest.cpp             # 03-03 テスト
├── ProcessTest.cpp          # 03-04 テスト
├── FileTest.cpp             # 03-05 テスト
├── ThreadingTest.cpp        # 05-xx テスト
├── StringUtilsTest.cpp      # 10-01 テスト
├── AlignmentTest.cpp        # 10-02 テスト
├── HashTest.cpp             # 10-03 テスト
└── Mocks/
    └── MockPlatformMemory.h # OS APIモック
```

### テストカテゴリ

| カテゴリ | 内容 | 実行タイミング |
|----------|------|----------------|
| ユニットテスト | 単一関数/クラスの検証 | 毎コミット |
| 統合テスト | モジュール間連携 | Phase完了時 |
| ストレステスト | マルチスレッド競合 | リリース前 |

### OS APIモック戦略

```cpp
// テスト用のOS API抽象化
struct IPlatformMemoryProvider
{
    virtual void* VirtualAlloc(...) = 0;
    virtual bool VirtualFree(...) = 0;
};

// 本番: WindowsPlatformMemoryProvider（OS API直接呼び出し）
// テスト: MockPlatformMemoryProvider（失敗シミュレーション可能）
```

- **依存性注入**: テスト時にモックプロバイダを注入
- **失敗シミュレーション**: OOM、権限エラー等を意図的に発生
- **呼び出し記録**: API呼び出し順序・引数の検証

## エラー処理ガイドライン

### エラー処理方針

| 状況 | 方針 | 例 |
|------|------|-----|
| プログラマエラー | フェイルファスト（assert） | 不正なアライメント値 |
| 環境エラー | 回復試行 → Result返却 | メモリ不足、ファイル不存在 |
| 致命的エラー | ログ出力 → クラッシュ | ヒープ破損、スタックオーバーフロー |

### エラー回復戦略

```
Malloc::Alloc() 失敗時:
  1. GC/Trimを試行
  2. 再度割り当て試行
  3. 失敗 → MallocError::OutOfMemory設定
  4. OOMハンドラ呼び出し（登録されている場合）
  5. デフォルト: nullptr返却（TryAlloc）/ クラッシュ（Alloc）
```

### OS APIエラーマッピング

| Windows API | HALエラー | 回復可能 |
|-------------|-----------|---------|
| `ERROR_NOT_ENOUGH_MEMORY` | `OutOfMemory` | △（GC後再試行） |
| `ERROR_INVALID_PARAMETER` | `InvalidArgument` | ✗（プログラマエラー） |
| `ERROR_ACCESS_DENIED` | `AccessDenied` | ✗ |
| `ERROR_FILE_NOT_FOUND` | `FileNotFound` | △（パス確認） |

### デバッグ支援

- **NS_MALLOC_DEBUG=1**: ガードバイト、DoubleFree検出有効
- **NS_HAL_VERBOSE=1**: OS API呼び出しをログ出力
- **NS_CHECK_SLOW**: 重いバリデーション（Debugビルドのみ）

## パフォーマンスガイドライン

### 計算量

| 操作 | 計算量 | 備考 |
|------|--------|------|
| `Malloc::Alloc` | O(1) 償却 | フリーリスト使用時 |
| `Malloc::Free` | O(1) | |
| `GetTypeHash` | O(n) | 文字列長に比例 |
| `Crc32` | O(n) | テーブル参照、SIMD最適化可 |
| `PlatformString::Strcmp` | O(n) | 最悪ケース |

### スレッド競合

| コンポーネント | 競合対策 | ロック粒度 |
|---------------|----------|-----------|
| `MallocAnsi` | グローバルロック | 粗い（将来TLS対応） |
| `LowLevelMemTracker` | thread_local | なし |
| `ConsoleManager` | RWロック | 中（登録時のみ書き込み） |
| `PlatformAtomics` | ロックフリー | なし |

### キャッシュ最適化

```cpp
// ✓ 良い: キャッシュライン境界を意識
struct alignas(64) PerThreadData { ... };

// ✗ 悪い: False Sharing発生
struct SharedCounters {
    std::atomic<int> counter1;  // 同一キャッシュラインに配置
    std::atomic<int> counter2;  // 別スレッドが頻繁にアクセス
};
```

### 最適化優先度

| 優先度 | 対象 | 理由 |
|--------|------|------|
| 高 | Malloc/Free | 全コードパスで使用 |
| 高 | Atomics | ロック実装の基盤 |
| 中 | Hash | コンテナ操作で頻繁に呼ばれる |
| 低 | StackWalk | デバッグ時のみ |

## ファイル配置規則

```
source/engine/hal/
├── Public/
│   ├── HAL/                    # 統一インターフェース
│   │   ├── Platform.h          # プラットフォームマクロ
│   │   ├── PlatformTypes.h     # 型エントリポイント
│   │   ├── PlatformMacros.h    # 関数属性マクロ
│   │   ├── PlatformMemory.h    # メモリ（エントリポイント）
│   │   ├── PlatformAtomics.h   # アトミック（エントリポイント）
│   │   └── ...
│   ├── GenericPlatform/        # 汎用実装（継承元）
│   │   ├── GenericPlatformTypes.h
│   │   ├── GenericPlatformMemory.h
│   │   └── ...
│   └── Windows/                # Windows固有
│       ├── WindowsPlatformTypes.h
│       ├── WindowsPlatformMemory.h
│       └── ...
└── Private/
    └── Windows/                # Windows実装
        ├── WindowsPlatformMemory.cpp
        └── ...
```

## 命名規則（UE5との差異）

| UE5 | NS-ENGINE | 理由 |
|-----|-----------|------|
| `FMalloc` | `Malloc` | Fプレフィックス不使用 |
| `ELLMTag` | `LLMTag` | Eプレフィックス不使用 |
| `Malloc()` | `Alloc()` | クラス名との衝突回避 |
| `TryMalloc()` | `TryAlloc()` | 同上 |
| `FChar` | `TChar<T>` | テンプレートはTプレフィックス |

## サブ計画

### Part1: アーキテクチャ基盤（4計画）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 01-01 | [ディレクトリ構造](01-01-directory-structure.md) | pending | 0（ディレクトリのみ）|
| 01-02 | [プラットフォームマクロ](01-02-platform-macros.md) | pending | 1 |
| 01-03 | [COMPILED_PLATFORM_HEADER](01-03-compiled-platform-header.md) | pending | 1 |
| 01-04 | [プラットフォーム型](01-04-platform-types.md) | pending | 3 |

### Part2: プリプロセッサ・型システム（3計画）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 02-01 | [機能フラグマクロ](02-01-feature-flags.md) | pending | 0（Platform.hに追加）|
| 02-02 | [コンパイラマクロ](02-02-compiler-macros.md) | pending | 0（Platform.hに追加）|
| 02-03 | [関数呼び出し規約](02-03-calling-conventions.md) | pending | 1 |

### Part3: HALコンポーネント（10計画 ← 5から分割）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 03-01a | [メモリ型定義](03-01a-memory-types.md) | pending | 1 |
| 03-01b | [Windowsメモリ実装](03-01b-windows-memory.md) | pending | 3 |
| 03-02a | [アトミックインターフェース](03-02a-atomics-interface.md) | pending | 1 |
| 03-02b | [Windowsアトミック実装](03-02b-windows-atomics.md) | pending | 2 |
| 03-03a | [時間インターフェース](03-03a-time-interface.md) | pending | 1 |
| 03-03b | [Windows時間実装](03-03b-windows-time.md) | pending | 3 |
| 03-04a | [プロセスインターフェース](03-04a-process-interface.md) | pending | 1 |
| 03-04b | [Windowsプロセス実装](03-04b-windows-process.md) | pending | 3 |
| 03-05a | [ファイルインターフェース](03-05a-file-interface.md) | pending | 1 |
| 03-05b | [Windowsファイル実装](03-05b-windows-file.md) | pending | 3 |

### Part4: ビルドシステム（1計画）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 04-01 | [premake HALプロジェクト](04-01-premake-hal.md) | pending | 0（premake5.lua変更）|

### Part5: スレッディング（6計画 ← 4から分割）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 05-01 | [PlatformAffinity](05-01-platform-affinity.md) | pending | 3 |
| 05-02 | [PlatformTLS](05-02-platform-tls.md) | pending | 3 |
| 05-03a | [スタックウォークインターフェース](05-03a-stackwalk-interface.md) | pending | 1 |
| 05-03b | [Windowsスタックウォーク実装](05-03b-windows-stackwalk.md) | pending | 3 |
| 05-04a | [クラッシュコンテキストインターフェース](05-04a-crashcontext-interface.md) | pending | 1 |
| 05-04b | [Windowsクラッシュコンテキスト実装](05-04b-windows-crashcontext.md) | pending | 3 |

### Part6: メモリアロケータ（6計画 ← 3から分割、直列実装必須）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 06-01a | [Malloc基底クラス](06-01a-malloc-base.md) | pending | 1 |
| 06-01b | [GMallocグローバル](06-01b-gmalloc-global.md) | pending | 0（追加）|
| 06-02a | [MallocAnsiクラス定義](06-02a-malloc-ansi-class.md) | pending | 1 |
| 06-02b | [MallocAnsi実装](06-02b-malloc-ansi-impl.md) | pending | 1 |
| 06-03a | [Memory静的API](06-03a-memory-static-api.md) | pending | 1 |
| 06-03b | [Memory API実装](06-03b-memory-api-impl.md) | pending | 1 |

### Part7: メモリトラッカー（4計画 ← 2から分割、直列実装必須）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 07-01a | [LLMタグ列挙](07-01a-llm-tag-enum.md) | pending | 1 |
| 07-01b | [LLMTrackerクラス](07-01b-llm-tracker-class.md) | pending | 0（追加）|
| 07-02a | [LLMScopeクラス](07-02a-llm-scope-class.md) | pending | 0（追加）|
| 07-02b | [LLMScopeマクロ](07-02b-llm-scope-macros.md) | pending | 1 |

### Part8: デバッグツール（6計画 ← 2から分割、並列実装可能）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 08-01a | [OutputDeviceインターフェース](08-01a-output-device-interface.md) | pending | 1 |
| 08-01b | [OutputDeviceDebug実装](08-01b-output-device-debug.md) | pending | 1 |
| 08-02a | [ScopedNamedEventクラス](08-02a-named-event-class.md) | pending | 1 |
| 08-02b | [名前付きイベントマクロ](08-02b-named-event-macros.md) | pending | 0（追加）|
| 08-03a | [ConsoleVariableインターフェース](08-03a-console-variable-interface.md) | pending | 1 |
| 08-03b | [ConsoleManager実装](08-03b-console-manager.md) | pending | 2 |

### Part9: プラットフォーム固有（4計画 ← 2から分割、並列実装可能）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 09-01a | [CPUInfo構造体](09-01a-cpu-info-struct.md) | pending | 1 |
| 09-01b | [WindowsPlatformMisc実装](09-01b-windows-platform-misc.md) | pending | 2 |
| 09-02a | [Windows COM初期化](09-02a-windows-com.md) | pending | 1 |
| 09-02b | [Windowsレジストリ・バージョン](09-02b-windows-registry-version.md) | pending | 0（追加）|

### Part10: ユーティリティ（6計画 ← 3から分割、並列実装可能）

| # | 計画 | 状態 | 作成ファイル数 |
|---|------|------|--------------|
| 10-01a | [TCharテンプレート](10-01a-tchar-template.md) | pending | 1 |
| 10-01b | [PlatformString文字列操作](10-01b-platform-string.md) | pending | 2 |
| 10-02a | [アライメントテンプレート](10-02a-alignment-templates.md) | pending | 1 |
| 10-03a | [GetTypeHashテンプレート](10-03a-gettypehash.md) | pending | 1 |
| 10-03b | [CRC32実装](10-03b-crc32.md) | pending | 1 |
| 10-04 | [スタックアロケーション](10-04-stack-allocation.md) | pending | 1 |

---

## 計画サマリー

| Part | 計画数 | 実装順序 |
|------|-------|---------|
| Part1 | 4 | 直列（基盤） |
| Part2 | 3 | 直列（基盤） |
| Part3 | 10 | 並列可能 |
| Part4 | 1 | Part2後 |
| Part5 | 6 | 並列可能 |
| Part6 | 6 | 直列 |
| Part7 | 4 | Part6後、直列 |
| Part8 | 6 | 並列可能 |
| Part9 | 4 | 並列可能 |
| Part10 | 6 | 並列可能 |
| **合計** | **50** | |

## 検証方法

### Phase別検証

| Phase | 検証内容 | 合格基準 |
|-------|----------|----------|
| Phase 1 | ビルド成功、マクロ展開確認 | `PLATFORM_WINDOWS == 1` |
| Phase 2A/2B | ユニットテスト | 各コンポーネント90%以上パス |
| Phase 3 | メモリテスト、Valgrind相当 | リーク0、破損検出動作 |
| Phase 4 | 統合テスト | 全機能の連携動作 |

### テスト実行

```bash
# 全HALテスト実行
powershell -Command "& cmd /c 'tools\@run_tests.cmd HAL'"

# 特定テストのみ
powershell -Command "& cmd /c 'tools\@run_tests.cmd HAL --filter=Memory*'"

# ストレステスト（マルチスレッド）
powershell -Command "& cmd /c 'tools\@run_tests.cmd HAL --stress'"
```

### 検証チェックリスト

- [ ] Windows x64 Debug ビルド成功
- [ ] Windows x64 Release ビルド成功
- [ ] 全ユニットテストパス
- [ ] メモリリークなし（Debug終了時レポート）
- [ ] ガードバイト破損検出が動作（意図的破損で検出）
- [ ] DoubleFree検出が動作
- [ ] マルチスレッドストレステスト通過（10分間）

## 参考ドキュメント

- [HAL_Design_Principles.md](docs/HAL_Design_Principles.md)
- [Platform_Abstraction_Layer_Part1.md](docs/Platform_Abstraction_Layer_Part1.md) 〜 Part10.md
