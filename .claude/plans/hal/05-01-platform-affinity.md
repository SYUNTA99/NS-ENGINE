# 05-01: PlatformAffinity

## 目的

スレッドアフィニティ・優先度管理を実装する。

## 参考

[Platform_Abstraction_Layer_Part5.md](docs/Platform_Abstraction_Layer_Part5.md) - セクション1「スレッディング詳細」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint64`, `uint32`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformAffinity.h`
- `source/engine/hal/Public/Windows/WindowsPlatformAffinity.h`
- `source/engine/hal/Private/Windows/WindowsPlatformAffinity.cpp`
- `source/engine/hal/Public/HAL/PlatformAffinity.h`

## TODO

- [ ] `ThreadPriority` 列挙型
- [ ] `ThreadType` 列挙型（用途別分類）
- [ ] `GenericPlatformAffinity` 構造体（アフィニティマスク取得）
- [ ] `WindowsPlatformAffinity` 構造体（実バインドAPI）
- [ ] `PlatformAffinity.h` エントリポイント

## 実装内容

```cpp
// GenericPlatformAffinity.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"

namespace NS
{
    /// スレッド優先度
    ///
    /// OSのスレッド優先度にマッピングされる。
    /// 実際の優先度値はプラットフォーム依存。
    enum class ThreadPriority
    {
        TimeCritical,       ///< 最高優先度（オーディオ等）
        Highest,            ///< 非常に高い
        AboveNormal,        ///< 通常より高い
        Normal,             ///< 通常（デフォルト）
        BelowNormal,        ///< 通常より低い
        Lowest,             ///< 最低優先度
        SlightlyBelowNormal ///< 通常よりわずかに低い（バックグラウンド処理）
    };

    /// スレッド用途タイプ
    ///
    /// アフィニティマスク取得時の用途指定。
    /// プラットフォームごとに最適なコア配置を提供。
    enum class ThreadType
    {
        MainGame,           ///< メインゲームスレッド（コア0優先）
        Rendering,          ///< レンダリングスレッド
        RHI,                ///< RHIコマンド生成スレッド
        TaskGraph,          ///< タスクグラフワーカー
        Pool,               ///< 汎用スレッドプール
        Audio,              ///< オーディオ処理（低レイテンシ）
        Loading,            ///< アセットロード（I/Oバウンド）
        Background,         ///< バックグラウンド処理（低優先度）
        Count
    };

    /// CPUトポロジ情報
    struct CPUTopology
    {
        uint32 physicalCoreCount;       ///< 物理コア数
        uint32 logicalProcessorCount;   ///< 論理プロセッサ数（HT含む）
        uint32 performanceCoreCount;    ///< Pコア数（ハイブリッドCPU）
        uint32 efficiencyCoreCount;     ///< Eコア数（ハイブリッドCPU）
        uint64 performanceCoreMask;     ///< Pコアのビットマスク
        uint64 efficiencyCoreMask;      ///< Eコアのビットマスク
        bool isHybridCPU;               ///< ハイブリッドCPU（Intel 12th+, ARM big.LITTLE）
    };

    /// プラットフォーム非依存のアフィニティ管理
    ///
    /// ## スレッドセーフティ
    ///
    /// 全関数はスレッドセーフ（読み取り専用または内部同期）。
    ///
    /// ## アフィニティマスク
    ///
    /// ビットマスクで実行可能なCPUコアを指定。
    /// - bit N = 1: コアNで実行可能
    /// - 0xFFFFFFFFFFFFFFFF: 全コアで実行可能（制限なし）
    struct GenericPlatformAffinity
    {
        // ========== アフィニティマスク取得 ==========

        /// スレッドタイプに応じたアフィニティマスク取得
        ///
        /// @param type スレッド用途
        /// @return アフィニティマスク
        static uint64 GetAffinityMask(ThreadType type);

        /// 制限なしマスク取得
        static uint64 GetNoAffinityMask() { return 0xFFFFFFFFFFFFFFFF; }

        // ========== 個別マスク（後方互換） ==========

        static uint64 GetMainGameMask() { return GetAffinityMask(ThreadType::MainGame); }
        static uint64 GetRenderingThreadMask() { return GetAffinityMask(ThreadType::Rendering); }
        static uint64 GetRHIThreadMask() { return GetAffinityMask(ThreadType::RHI); }
        static uint64 GetTaskGraphThreadMask() { return GetAffinityMask(ThreadType::TaskGraph); }
        static uint64 GetPoolThreadMask() { return GetAffinityMask(ThreadType::Pool); }

        // ========== 優先度 ==========

        /// スレッドタイプのデフォルト優先度取得
        static ThreadPriority GetDefaultPriority(ThreadType type);

        /// 汎用デフォルト優先度
        static ThreadPriority GetDefaultPriority() { return ThreadPriority::Normal; }

        // ========== トポロジ情報 ==========

        /// CPUトポロジ取得
        ///
        /// @return CPUトポロジ情報
        /// @note 初回呼び出し時にキャッシュされる
        static const CPUTopology& GetCPUTopology();

        // ========== 実行時バインド ==========

        /// 現在のスレッドにアフィニティを設定
        ///
        /// @param mask アフィニティマスク
        /// @return 設定成功した場合true
        static bool SetCurrentThreadAffinity(uint64 mask);

        /// 現在のスレッドの優先度を設定
        ///
        /// @param priority 優先度
        /// @return 設定成功した場合true
        static bool SetCurrentThreadPriority(ThreadPriority priority);

        /// 現在のスレッドが実行中のコアIDを取得
        ///
        /// @return コアID（0〜logicalProcessorCount-1）
        static uint32 GetCurrentProcessorNumber();

        /// スレッドをスリープ（ミリ秒）
        static void Sleep(uint32 milliseconds);

        /// スレッドをスリープ（マイクロ秒、スピンウェイト）
        static void SleepMicroseconds(uint32 microseconds);

        /// 他のスレッドに実行権を譲る
        static void YieldThread();
    };
}

// WindowsPlatformAffinity.h
#pragma once

#include "GenericPlatform/GenericPlatformAffinity.h"

namespace NS
{
    /// Windows固有のアフィニティ管理
    ///
    /// ハイブリッドCPU（Intel 12th+）対応：
    /// - ゲームスレッド/レンダリング → Pコア優先
    /// - バックグラウンド/ロード → Eコア優先
    struct WindowsPlatformAffinity : public GenericPlatformAffinity
    {
        /// アフィニティマスク取得（Windows最適化）
        static uint64 GetAffinityMask(ThreadType type);

        /// CPUトポロジ取得（Windows実装）
        static const CPUTopology& GetCPUTopology();

        /// 現在のスレッドにアフィニティを設定
        static bool SetCurrentThreadAffinity(uint64 mask);

        /// 現在のスレッドの優先度を設定
        static bool SetCurrentThreadPriority(ThreadPriority priority);

        /// 現在のプロセッサ番号取得
        static uint32 GetCurrentProcessorNumber();

        /// スリープ
        static void Sleep(uint32 milliseconds);
        static void SleepMicroseconds(uint32 microseconds);
        static void YieldThread();

    private:
        /// Windows優先度値に変換
        static int32 ToWindowsPriority(ThreadPriority priority);

        /// トポロジ初期化
        static void InitializeTopology();

        static CPUTopology s_topology;
        static bool s_topologyInitialized;
    };

    typedef WindowsPlatformAffinity PlatformAffinity;
}
```

```cpp
// WindowsPlatformAffinity.cpp
#include "Windows/WindowsPlatformAffinity.h"
#include <windows.h>

namespace NS
{
    CPUTopology WindowsPlatformAffinity::s_topology = {};
    bool WindowsPlatformAffinity::s_topologyInitialized = false;

    void WindowsPlatformAffinity::InitializeTopology()
    {
        if (s_topologyInitialized) return;

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        s_topology.logicalProcessorCount = sysInfo.dwNumberOfProcessors;

        // 物理コア数取得
        DWORD length = 0;
        GetLogicalProcessorInformation(nullptr, &length);
        auto* buffer = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(
            malloc(length));

        if (GetLogicalProcessorInformation(buffer, &length))
        {
            DWORD offset = 0;
            uint32 physicalCores = 0;
            while (offset < length)
            {
                auto* ptr = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(
                    reinterpret_cast<uint8*>(buffer) + offset);
                if (ptr->Relationship == RelationProcessorCore)
                {
                    physicalCores++;
                }
                offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            }
            s_topology.physicalCoreCount = physicalCores;
        }
        free(buffer);

        // ハイブリッドCPU検出（簡易版）
        // TODO: GetSystemCpuSetInformation()でP/Eコア検出
        s_topology.isHybridCPU = false;
        s_topology.performanceCoreCount = s_topology.physicalCoreCount;
        s_topology.efficiencyCoreCount = 0;
        s_topology.performanceCoreMask = (1ULL << s_topology.logicalProcessorCount) - 1;
        s_topology.efficiencyCoreMask = 0;

        s_topologyInitialized = true;
    }

    uint64 WindowsPlatformAffinity::GetAffinityMask(ThreadType type)
    {
        InitializeTopology();

        const uint64 allCores = (1ULL << s_topology.logicalProcessorCount) - 1;

        // ハイブリッドCPU対応
        if (s_topology.isHybridCPU)
        {
            switch (type)
            {
            case ThreadType::MainGame:
            case ThreadType::Rendering:
            case ThreadType::RHI:
            case ThreadType::Audio:
                return s_topology.performanceCoreMask;

            case ThreadType::Loading:
            case ThreadType::Background:
                return s_topology.efficiencyCoreMask;

            default:
                return allCores;
            }
        }

        // 通常CPU：全コア使用
        return allCores;
    }

    bool WindowsPlatformAffinity::SetCurrentThreadAffinity(uint64 mask)
    {
        DWORD_PTR result = SetThreadAffinityMask(GetCurrentThread(), mask);
        return result != 0;
    }

    bool WindowsPlatformAffinity::SetCurrentThreadPriority(ThreadPriority priority)
    {
        return SetThreadPriority(GetCurrentThread(), ToWindowsPriority(priority)) != 0;
    }

    int32 WindowsPlatformAffinity::ToWindowsPriority(ThreadPriority priority)
    {
        switch (priority)
        {
        case ThreadPriority::TimeCritical:      return THREAD_PRIORITY_TIME_CRITICAL;
        case ThreadPriority::Highest:           return THREAD_PRIORITY_HIGHEST;
        case ThreadPriority::AboveNormal:       return THREAD_PRIORITY_ABOVE_NORMAL;
        case ThreadPriority::Normal:            return THREAD_PRIORITY_NORMAL;
        case ThreadPriority::BelowNormal:       return THREAD_PRIORITY_BELOW_NORMAL;
        case ThreadPriority::Lowest:            return THREAD_PRIORITY_LOWEST;
        case ThreadPriority::SlightlyBelowNormal: return THREAD_PRIORITY_BELOW_NORMAL;
        default:                                return THREAD_PRIORITY_NORMAL;
        }
    }

    uint32 WindowsPlatformAffinity::GetCurrentProcessorNumber()
    {
        return ::GetCurrentProcessorNumber();
    }

    void WindowsPlatformAffinity::Sleep(uint32 milliseconds)
    {
        ::Sleep(milliseconds);
    }

    void WindowsPlatformAffinity::YieldThread()
    {
        ::SwitchToThread();
    }
}
```

## 検証

- `GetAffinityMask(ThreadType::MainGame)` が有効なマスクを返す
- `SetCurrentThreadAffinity()` が成功を返す
- `GetCPUTopology()` が正しいコア数を報告
- `SetCurrentThreadPriority()` が優先度を変更
- ハイブリッドCPUでP/Eコア分離が機能（対応環境）

## 注意事項

- アフィニティ設定は権限が必要な場合がある
- ハイブリッドCPU検出はWindows 10以降でフル対応
- `SleepMicroseconds` はスピンウェイトなのでCPU負荷に注意

## 次のサブ計画

→ 05-02: PlatformTLS
