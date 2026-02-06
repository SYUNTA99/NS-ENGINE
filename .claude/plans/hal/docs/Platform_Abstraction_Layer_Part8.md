# Unreal Engine 5.7 プラットフォーム抽象化レイヤー (HAL) 完全解説

## 第8部：デバッグ・診断ツール

---

## 目次

1. [概要](#1-概要)
2. [コンソール変数システム](#2-コンソール変数システム)
3. [スレッド管理とスタックトレース](#3-スレッド管理とスタックトレース)
4. [クラッシュコンテキストと終了コード](#4-クラッシュコンテキストと終了コード)
5. [名前付きイベント](#5-名前付きイベント)
6. [I/Oパフォーマンストラッキング](#6-ioパフォーマンストラッキング)
7. [メモリスコープ統計](#7-メモリスコープ統計)
8. [フィードバックコンテキスト](#8-フィードバックコンテキスト)

---

## 1. 概要

UE5.7のHALには、デバッグと診断のための多様なツールが含まれています。これらはプラットフォーム間で一貫したインターフェースを提供しながら、低レベルのシステム情報へのアクセスを可能にします。

```
┌─────────────────────────────────────────────────────────────────────────┐
│                  デバッグ・診断ツール階層構造                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【アプリケーション診断層】                                              │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │  IConsoleManager    - コンソール変数/コマンド管理                  │ │
│  │  OutputDevice      - 出力デバイス抽象化                          │ │
│  │  FeedbackContext   - ユーザーフィードバック                       │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│  【スレッド・実行診断層】                                                │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │  ThreadManager          - スレッド管理・列挙                     │ │
│  │  ThreadHeartBeat        - スレッドハングアップ検出               │ │
│  │  PlatformStackWalk      - スタックウォーク・シンボル解決         │ │
│  │  ScopedNamedEvent       - 名前付きイベントプロファイリング        │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│  【クラッシュ・エラー診断層】                                            │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │  GenericPlatformCrashContext - クラッシュコンテキスト収集        │ │
│  │  ECrashContextType            - クラッシュタイプ分類              │ │
│  │  ECrashExitCodes             - 終了コード定義                     │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│  【I/O・リソース診断層】                                                 │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │  DiskUtilizationTracker    - ディスクI/O追跡                     │ │
│  │  ScopedMemoryStats         - メモリ統計スコープ                  │ │
│  │  SharedMemoryTracker       - 共有メモリ追跡 (Unix)               │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. コンソール変数システム

### IConsoleManager インターフェース

`IConsoleManager`は、コンソール変数とコマンドを管理するための中央システムです。

```cpp
// IConsoleManager.h

class IConsoleManager
{
public:
    // シングルトンアクセス
    static IConsoleManager& Get();

    // コンソール変数登録
    virtual IConsoleVariable* RegisterConsoleVariable(
        const TCHAR* Name,
        int32 DefaultValue,
        const TCHAR* Help,
        uint32 Flags = ECVF_Default
    ) = 0;

    // コンソールコマンド登録
    virtual IConsoleCommand* RegisterConsoleCommand(
        const TCHAR* Name,
        const TCHAR* Help,
        const ConsoleCommandDelegate& Command,
        uint32 Flags = ECVF_Default
    ) = 0;

    // 変数検索
    virtual IConsoleVariable* FindConsoleVariable(
        const TCHAR* Name,
        bool bTrackFrequentCalls = true
    ) const = 0;

    // 変数の登録解除
    virtual void UnregisterConsoleObject(
        IConsoleObject* Object,
        bool bKeepState = true
    ) = 0;
};
```

### EConsoleVariableFlags

コンソール変数のフラグシステムは、変数の動作と優先度を制御します。

```cpp
enum EConsoleVariableFlags
{
    // ======== 基本フラグ (0x0000-0x0FFF) ========

    ECVF_FlagMask = 0x0000FFFF,     // フラグマスク
    ECVF_Default = 0x0,              // デフォルト（フラグなし）

    ECVF_Cheat = 0x1,               // チートフラグ（Shipping非表示）
    ECVF_ReadOnly = 0x4,            // 読み取り専用（コンソールから変更不可）
    ECVF_Unregistered = 0x8,        // 登録解除済み（DLLアンロード用）
    ECVF_CreatedFromIni = 0x10,     // INIから作成（後で登録される）
    ECVF_RenderThreadSafe = 0x20,   // レンダースレッドセーフ

    ECVF_Scalability = 0x40,        // スケーラビリティ設定
    ECVF_ScalabilityGroup = 0x80,   // スケーラビリティグループ（sg.で始まる）
    ECVF_Preview = 0x100,           // プレビュー対象

    ECVF_GeneralShaderChange = 0x200,  // 全プラットフォームシェーダー変更
    ECVF_MobileShaderChange = 0x400,   // モバイルシェーダー変更
    ECVF_DesktopShaderChange = 0x800,  // デスクトップシェーダー変更

    ECVF_ExcludeFromPreview = 0x1000,  // プレビュー除外
    ECVF_SaveForNextBoot = 0x2000,     // 次回起動用保存（ホットフィックス用）

    // ======== 設定フラグ (0x00FF0000) ========

    ECVF_SetFlagMask = 0x00FF0000,
    ECVF_Set_NoSinkCall_Unsafe = 0x00010000,   // シンクコールなし（高速だが危険）
    ECVF_Set_SetOnly_Unsafe = 0x00020000,      // 設定のみ（最小操作）
    ECVF_Set_ReplaceExistingTag = 0x00040000,  // 既存タグ置換

    // ======== SetBy優先度 (0xFF000000) ========

    ECVF_SetByMask = 0xFF000000,

    // 優先度順（低→高）
    ECVF_SetByConstructor = 0x00000000,        // コンストラクタ（最低）
    ECVF_SetByScalability = 0x02000000,        // スケーラビリティ
    ECVF_SetByGameSetting = 0x03000000,        // ゲーム設定
    ECVF_SetByProjectSetting = 0x04000000,     // プロジェクト設定
    ECVF_SetBySystemSettingsIni = 0x05000000,  // システム設定INI
    ECVF_SetByPluginLowPriority = 0x06000000,  // プラグイン（低優先）
    ECVF_SetByDeviceProfile = 0x07000000,      // デバイスプロファイル
    ECVF_SetByPluginHighPriority = 0x08000000, // プラグイン（高優先）
    ECVF_SetByGameOverride = 0x09000000,       // ゲームオーバーライド
    ECVF_SetByConsoleVariablesIni = 0x0A000000,// ConsoleVariables.ini
    ECVF_SetByHotfix = 0x0B000000,             // ホットフィックス
    ECVF_SetByPreview = 0x0C000000,            // プレビュー
    ECVF_SetByCommandline = 0x0D000000,        // コマンドライン
    ECVF_SetByCode = 0x0E000000,               // コード
    ECVF_SetByConsole = 0x0F000000,            // コンソール（最高）
};
```

### SetBy優先度システム

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    SetBy 優先度階層                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  優先度（低→高）                                                         │
│                                                                         │
│  ┌─────────────────┐                                                    │
│  │ Constructor     │ ← 変数作成時のデフォルト値                        │
│  ├─────────────────┤                                                    │
│  │ Scalability    │ ← Scalability.ini (配列型)                        │
│  ├─────────────────┤                                                    │
│  │ GameSetting    │ ← エンジンレベルのゲーム設定                       │
│  ├─────────────────┤                                                    │
│  │ ProjectSetting │ ← プロジェクトUI/INI設定                           │
│  ├─────────────────┤                                                    │
│  │ SystemSettings │ ← Engine.ini [ConsoleVariables]                   │
│  ├─────────────────┤                                                    │
│  │ PluginLow      │ ← 動的プラグイン（低優先、配列型）                 │
│  ├─────────────────┤                                                    │
│  │ DeviceProfile  │ ← DeviceProfiles.ini (配列型)                     │
│  ├─────────────────┤                                                    │
│  │ PluginHigh     │ ← 動的プラグイン（高優先、配列型）                 │
│  ├─────────────────┤                                                    │
│  │ GameOverride   │ ← GameUserSettings オーバーライド                  │
│  ├─────────────────┤                                                    │
│  │ CVarsIni       │ ← ConsoleVariables.ini                            │
│  ├─────────────────┤                                                    │
│  │ Hotfix         │ ← ホットフィックス（配列型）                       │
│  ├─────────────────┤                                                    │
│  │ Preview        │ ← エディタプレビュー（配列型）                     │
│  ├─────────────────┤                                                    │
│  │ Commandline    │ ← コマンドライン引数                               │
│  ├─────────────────┤                                                    │
│  │ Code           │ ← 高優先度コード                                   │
│  ├─────────────────┤                                                    │
│  │ Console        │ ← インタラクティブコンソール（最高）               │
│  └─────────────────┘                                                    │
│                                                                         │
│  注: "配列型" は複数値を履歴として保持可能                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 使用例

```cpp
// コンソール変数の登録
static TAutoConsoleVariable<int32> CVarMyFeature(
    TEXT("r.MyFeature"),
    1,
    TEXT("0: 無効, 1: 有効"),
    ECVF_RenderThreadSafe | ECVF_Scalability
);

// コンソール変数の参照（読み取り専用）
static const auto* CVarRef = IConsoleManager::Get().FindTConsoleVariableDataInt(
    TEXT("r.MyFeature")
);
int32 Value = CVarRef ? CVarRef->GetValueOnAnyThread() : 1;

// コンソールコマンドの登録
static AutoConsoleCommand CVarMyCommand(
    TEXT("MyProject.DoSomething"),
    TEXT("説明文"),
    ConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<String>& Args)
    {
        // コマンド実行
    })
);

// SetBy情報の取得
const TCHAR* SetByName = GetConsoleVariableSetByName(
    (EConsoleVariableFlags)(CVarMyFeature->GetFlags() & ECVF_SetByMask)
);
```

---

## 3. スレッド管理とスタックトレース

### ThreadManager

```cpp
// ThreadManager.h

class ThreadManager
{
public:
    // シングルトンアクセス
    static ThreadManager& Get();

    // スレッド追加/削除
    void AddThread(uint32 ThreadId, RunnableThread* Thread);
    void RemoveThread(RunnableThread* Thread);

    // スレッド数取得
    int32 NumThreads() const;

    // スレッド名取得
    static const String& GetThreadName(uint32 ThreadId);

    // スレッド列挙
    void ForEachThread(TFunction<void(uint32 ThreadId, RunnableThread* Thread)> Func);

    // フェイクスレッドのティック
    void Tick();
};
```

### スタックバックトレース取得

```cpp
// PLATFORM_SUPPORTS_ALL_THREAD_BACKTRACES が有効なプラットフォームのみ
// (Windows, Mac)

#if PLATFORM_SUPPORTS_ALL_THREAD_BACKTRACES

struct ThreadStackBackTrace
{
    static constexpr uint32 ProgramCountersMaxStackSize = 100;
    typedef TArray<uint64, TFixedAllocator<ProgramCountersMaxStackSize>> ProgramCountersArray;

    uint32 ThreadId;
    String ThreadName;
    ProgramCountersArray ProgramCounters;
};

// 全スレッドのスタックトレース取得
void GetAllThreadStackBackTraces(TArray<ThreadStackBackTrace>& StackTraces);

// イテレータによるスタックトレース取得（メモリ割り当てなし、クラッシュ時向け）
void ForEachThreadStackBackTrace(
    TFunctionRef<bool(uint32 ThreadId, const TCHAR* ThreadName,
                      const TConstArrayView<uint64>& StackTrace)> Func
);

#endif
```

### PlatformStackWalk

```cpp
// PlatformStackWalk.h

struct ProgramCounterSymbolInfo
{
    ANSICHAR ModuleName[MAX_PATH];
    ANSICHAR FunctionName[MAX_FUNCTION_NAME_LENGTH];
    ANSICHAR Filename[MAX_PATH];
    int32 LineNumber;
    uint64 ProgramCounter;
    uint64 OffsetInModule;
    uint64 SymbolDisplacement;
};

class GenericPlatformStackWalk
{
public:
    // スタックウォーク
    static int32 CaptureStackBackTrace(
        uint64* BackTrace,
        uint32 MaxDepth,
        void* Context = nullptr
    );

    // シンボル解決
    static bool ProgramCounterToSymbolInfo(
        uint64 ProgramCounter,
        ProgramCounterSymbolInfo& OutSymbolInfo
    );

    // 人間が読める形式に変換
    static void StackWalkAndDump(
        ANSICHAR* HumanReadableString,
        SIZE_T HumanReadableStringSize,
        int32 IgnoreCount,
        void* Context = nullptr
    );
};
```

### 使用例

```cpp
// スレッドダンプの取得
void DumpAllThreadStacks()
{
#if PLATFORM_SUPPORTS_ALL_THREAD_BACKTRACES
    TArray<ThreadManager::ThreadStackBackTrace> StackTraces;
    ThreadManager::Get().GetAllThreadStackBackTraces(StackTraces);

    for (const auto& Trace : StackTraces)
    {
        UE_LOG(LogCore, Log, TEXT("Thread: %s (ID: %u)"),
               *Trace.ThreadName, Trace.ThreadId);

        for (uint64 PC : Trace.ProgramCounters)
        {
            ProgramCounterSymbolInfo SymbolInfo;
            if (PlatformStackWalk::ProgramCounterToSymbolInfo(PC, SymbolInfo))
            {
                UE_LOG(LogCore, Log, TEXT("  %s (%s:%d)"),
                       ANSI_TO_TCHAR(SymbolInfo.FunctionName),
                       ANSI_TO_TCHAR(SymbolInfo.Filename),
                       SymbolInfo.LineNumber);
            }
        }
    }
#endif
}
```

---

## 4. クラッシュコンテキストと終了コード

### ECrashContextType

```cpp
enum class ECrashContextType
{
    Crash,       // 一般的なクラッシュ
    Assert,      // アサート失敗
    Ensure,      // ensure失敗（続行可能）
    Stall,       // ストール検出
    GPUCrash,    // GPUクラッシュ
    Hang,        // ハングアップ検出
    OutOfMemory, // メモリ不足
    AbnormalShutdown, // 異常シャットダウン

    Max
};
```

### ECrashDumpMode

```cpp
enum class ECrashDumpMode
{
    Default,         // デフォルトダンプ
    FullDump,        // フルダンプ（要求時）
    FullDumpAlways,  // 常にフルダンプ
};
```

### ECrashExitCodes

```cpp
// PlatformMisc.h

namespace ECrashExitCodes
{
    // 標準終了コード
    constexpr int32 Success = 0;

    // クラッシュ関連
    constexpr int32 UnhandledException = 1;
    constexpr int32 Assert = 2;
    constexpr int32 Ensure = 3;
    constexpr int32 OutOfMemory = 139;  // SIGABRT相当
    constexpr int32 StackOverflow = 134;

    // GPUクラッシュ
    constexpr int32 GPUCrash = 4;
    constexpr int32 GPUHang = 5;

    // ハングアップ検出
    constexpr int32 ThreadHang = 6;
    constexpr int32 DeadlockDetected = 7;

    // 異常終了
    constexpr int32 AbnormalShutdown = 8;
    constexpr int32 UserInitiated = 9;

    // プラットフォーム固有
    constexpr int32 PlatformSpecificStart = 100;
};
```

### GenericPlatformCrashContext

```cpp
class GenericPlatformCrashContext
{
public:
    // クラッシュコンテキストの初期化
    void Initialize();

    // クラッシュ情報の設定
    void SetCrashType(ECrashContextType Type);
    void SetErrorMessage(const TCHAR* Message);
    void SetCallstack(uint64* BackTrace, int32 Depth);

    // スレッド情報の追加
    void AddThreadInfo(uint32 ThreadId, const TCHAR* ThreadName);

    // メモリ情報の追加
    void SetMemoryStats(
        uint64 UsedPhysical,
        uint64 AvailablePhysical,
        uint64 UsedVirtual,
        uint64 AvailableVirtual
    );

    // クラッシュレポートの生成
    void GenerateCrashReport();

    // ミニダンプの生成
    void WriteMiniDump(ECrashDumpMode DumpMode);

    // プロパティアクセス
    ECrashContextType GetCrashType() const;
    const TCHAR* GetErrorMessage() const;
};
```

### 使用例

```cpp
// クラッシュハンドラの設定
void MyGameModule::StartupModule()
{
    // クラッシュコールバックの登録
    CoreDelegates::OnHandleSystemError.AddLambda([](void* Context)
    {
        GenericPlatformCrashContext* CrashContext =
            GenericPlatformCrashContext::Get();

        if (CrashContext)
        {
            // カスタム情報を追加
            CrashContext->SetGameState(TEXT("InMatch"));
            CrashContext->SetPlayerCount(GetActivePlayerCount());
        }
    });
}
```

---

## 5. 名前付きイベント

### ScopedNamedEvent

プロファイリングツール（Unreal Insights、外部プロファイラ）で表示される名前付きイベントを作成します。

```cpp
// Misc/ScopedEvent.h

// 有効化フラグ
#ifndef ENABLE_NAMED_EVENTS
    #define ENABLE_NAMED_EVENTS (!UE_BUILD_SHIPPING)
#endif

// イベントカラー
namespace Color
{
    constexpr Color Red(255, 0, 0);
    constexpr Color Green(0, 255, 0);
    constexpr Color Blue(0, 0, 255);
    constexpr Color Yellow(255, 255, 0);
    constexpr Color Cyan(0, 255, 255);
    constexpr Color Magenta(255, 0, 255);
    constexpr Color Orange(255, 165, 0);
    constexpr Color Purple(128, 0, 128);
    constexpr Color Turquoise(64, 224, 208);
    constexpr Color Silver(192, 192, 192);
}

class ScopedNamedEvent
{
public:
    // 色指定コンストラクタ
    ScopedNamedEvent(const Color& Color, const TCHAR* Text);
    ScopedNamedEvent(const Color& Color, const ANSICHAR* Text);

    // 色なしコンストラクタ（デフォルト色使用）
    ScopedNamedEvent(const TCHAR* Text);

    ~ScopedNamedEvent();

private:
    // プラットフォーム固有のイベントハンドル
    void* PlatformHandle;
};

// マクロ
#if ENABLE_NAMED_EVENTS
    #define SCOPED_NAMED_EVENT(Name, Color) \
        ScopedNamedEvent PREPROCESSOR_JOIN(Event_, __LINE__)(Color, TEXT(#Name))
    #define SCOPED_NAMED_EVENT_TEXT(Text, Color) \
        ScopedNamedEvent PREPROCESSOR_JOIN(Event_, __LINE__)(Color, Text)
#else
    #define SCOPED_NAMED_EVENT(Name, Color)
    #define SCOPED_NAMED_EVENT_TEXT(Text, Color)
#endif
```

### 使用例

```cpp
void MyExpensiveFunction()
{
    SCOPED_NAMED_EVENT(MyExpensiveFunction, Color::Red);

    // 内部処理
    {
        SCOPED_NAMED_EVENT_TEXT(TEXT("SubTask A"), Color::Green);
        DoSubTaskA();
    }

    {
        SCOPED_NAMED_EVENT_TEXT(TEXT("SubTask B"), Color::Blue);
        DoSubTaskB();
    }
}
```

---

## 6. I/Oパフォーマンストラッキング

### DiskUtilizationTracker

```cpp
// DiskUtilizationTracker.h

class DiskUtilizationTracker
{
public:
    // 読み取り開始/終了
    void BeginRead(const String& Filename, int64 Size);
    void EndRead(const String& Filename, int64 ActualBytesRead, double Duration);

    // 書き込み開始/終了
    void BeginWrite(const String& Filename, int64 Size);
    void EndWrite(const String& Filename, int64 ActualBytesWritten, double Duration);

    // 統計取得
    double GetReadThroughput() const;      // バイト/秒
    double GetWriteThroughput() const;     // バイト/秒
    double GetAverageReadLatency() const;  // 秒
    double GetAverageWriteLatency() const; // 秒

    int64 GetTotalBytesRead() const;
    int64 GetTotalBytesWritten() const;
    int32 GetPendingReadCount() const;
    int32 GetPendingWriteCount() const;

    // 統計リセット
    void ResetStats();

    // シングルトン
    static DiskUtilizationTracker& Get();
};
```

### 使用例

```cpp
void LogDiskStats()
{
    DiskUtilizationTracker& Tracker = DiskUtilizationTracker::Get();

    UE_LOG(LogIO, Log, TEXT("Read Throughput: %.2f MB/s"),
           Tracker.GetReadThroughput() / (1024.0 * 1024.0));
    UE_LOG(LogIO, Log, TEXT("Write Throughput: %.2f MB/s"),
           Tracker.GetWriteThroughput() / (1024.0 * 1024.0));
    UE_LOG(LogIO, Log, TEXT("Pending Reads: %d"),
           Tracker.GetPendingReadCount());
}
```

---

## 7. メモリスコープ統計

### ScopedMemoryStats

メモリ操作の前後で統計を収集し、メモリ使用量の変化を追跡します。

```cpp
// MemoryBase.h

class ScopedMemoryStats
{
public:
    ScopedMemoryStats(const TCHAR* ScopeName);
    ~ScopedMemoryStats();

    // 現在のスコープ内での変化量取得
    int64 GetAllocatedDelta() const;
    int64 GetFreedDelta() const;
    int64 GetNetDelta() const;

    // 詳細統計
    struct Stats
    {
        int64 AllocCount;
        int64 FreeCount;
        int64 AllocatedBytes;
        int64 FreedBytes;
        int64 PeakBytes;
    };

    const Stats& GetStats() const;

private:
    Stats StartStats;
    const TCHAR* ScopeName;
};
```

### 使用例

```cpp
void LoadLevel(const String& LevelName)
{
    ScopedMemoryStats MemStats(TEXT("LoadLevel"));

    // レベルロード処理
    DoLevelLoad(LevelName);

    UE_LOG(LogLevel, Log, TEXT("Level '%s' loaded. Memory delta: %lld bytes"),
           *LevelName, MemStats.GetNetDelta());
}
```

---

## 8. フィードバックコンテキスト

### FeedbackContextAnsi

進捗表示とユーザーフィードバックのための出力コンテキストです。

```cpp
// FeedbackContextAnsi.h

class FeedbackContextAnsi : public FeedbackContext
{
public:
    // コンストラクタ
    FeedbackContextAnsi();

    // 進捗表示
    virtual void BeginSlowTask(const Text& Task, bool bShowCancelButton,
                               bool bShowProgressDialog = true) override;
    virtual void UpdateProgress(int32 Numerator, int32 Denominator) override;
    virtual void EndSlowTask() override;

    // ステータス更新
    virtual void StatusUpdate(int32 Numerator, int32 Denominator,
                              const Text& StatusText) override;
    virtual void StatusForceUpdate(int32 Numerator, int32 Denominator,
                                   const Text& StatusText) override;

    // キャンセル確認
    virtual bool ReceivedUserCancel() override;

    // Yes/No確認
    virtual bool YesNof(const Text& Question) override;

    // メッセージログ
    virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity,
                           const class Name& Category) override;

    // 警告/エラーカウント
    int32 GetWarningCount() const;
    int32 GetErrorCount() const;
};
```

### 使用例

```cpp
void ProcessAssets(FeedbackContext* Context, const TArray<UObject*>& Assets)
{
    if (Context)
    {
        Context->BeginSlowTask(
            Text::FromString(TEXT("Processing Assets...")),
            true,  // キャンセルボタン表示
            true   // 進捗ダイアログ表示
        );
    }

    for (int32 i = 0; i < Assets.Num(); ++i)
    {
        if (Context)
        {
            // キャンセル確認
            if (Context->ReceivedUserCancel())
            {
                break;
            }

            // 進捗更新
            Context->UpdateProgress(i, Assets.Num());
            Context->StatusUpdate(i, Assets.Num(),
                Text::Format(LOCTEXT("Processing", "Processing {0}..."),
                              Text::FromString(Assets[i]->GetName())));
        }

        ProcessAsset(Assets[i]);
    }

    if (Context)
    {
        Context->EndSlowTask();
    }
}
```

---

## 付録：診断ツール一覧

### HAL診断クラス一覧

| クラス | ヘッダー | 説明 |
|--------|----------|------|
| `IConsoleManager` | IConsoleManager.h | コンソール変数/コマンド管理 |
| `ThreadManager` | ThreadManager.h | スレッド管理・列挙 |
| `ThreadHeartBeat` | ThreadHeartBeat.h | スレッドハング検出 |
| `PlatformStackWalk` | PlatformStackWalk.h | スタックウォーク |
| `GenericPlatformCrashContext` | PlatformCrashContext.h | クラッシュコンテキスト |
| `ScopedNamedEvent` | ScopedEvent.h | 名前付きイベント |
| `DiskUtilizationTracker` | DiskUtilizationTracker.h | I/O追跡 |
| `ScopedMemoryStats` | MemoryBase.h | メモリ統計スコープ |
| `FeedbackContextAnsi` | FeedbackContextAnsi.h | フィードバック出力 |

### コンソールコマンド一覧

| コマンド | 説明 |
|----------|------|
| `stat threads` | スレッド統計表示 |
| `stat memory` | メモリ統計表示 |
| `stat llm` | LLMサマリー表示 |
| `dumpticks` | ティック情報ダンプ |
| `dumpslots` | TLSスロット情報ダンプ |
| `dumpthreads` | スレッド情報ダンプ |
| `debug crash` | 意図的クラッシュ（テスト用） |
| `debug assert` | 意図的アサート（テスト用） |
| `debug ensure` | 意図的ensure（テスト用） |
| `debug oom` | OOMシミュレーション |

---

## 関連ドキュメント

- [第3部：メモリ管理](Platform_Abstraction_Layer_Part3.md)
- [第5部：スレッディング](Platform_Abstraction_Layer_Part5.md)
- [第7部：Low Level Memory Tracker](Platform_Abstraction_Layer_Part7.md)
