# 03-01a: メモリ型定義

## 目的

メモリ統計・定数の型定義を行う（実装なし、ヘッダのみ）。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション1「メモリ管理」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint64`）

## 依存（HAL）

- 01-04 プラットフォーム型（`SIZE_T`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformMemory.h`

## TODO

- [ ] `PlatformMemoryStats` 構造体定義
- [ ] `PlatformMemoryConstants` 構造体定義
- [ ] `GenericPlatformMemory` インターフェース宣言（実装は03-01bで）

## 実装内容

```cpp
// GenericPlatformMemory.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"

namespace NS
{
    /// メモリ使用状況の統計情報
    ///
    /// GetStats()から返される値はスナップショット。
    /// マルチスレッド環境では値が即座に古くなる可能性がある。
    struct PlatformMemoryStats
    {
        uint64 availablePhysical;   ///< 利用可能な物理メモリ（バイト）
        uint64 availableVirtual;    ///< 利用可能な仮想メモリ（バイト）
        uint64 usedPhysical;        ///< 使用中の物理メモリ（バイト）
        uint64 usedVirtual;         ///< 使用中の仮想メモリ（バイト）
        uint64 peakUsedPhysical;    ///< ピーク物理メモリ使用量（バイト）
        uint64 peakUsedVirtual;     ///< ピーク仮想メモリ使用量（バイト）
    };

    /// システムメモリの定数情報
    ///
    /// Init()後は不変。スレッドセーフに読み取り可能。
    struct PlatformMemoryConstants
    {
        uint64 totalPhysical;       ///< 総物理メモリ（バイト）
        uint64 totalVirtual;        ///< 総仮想メモリ（バイト）
        SIZE_T pageSize;            ///< ページサイズ（通常4096）
        SIZE_T allocationGranularity; ///< 割り当て粒度（Windowsは64KB）
        SIZE_T cacheLineSize;       ///< CPUキャッシュラインサイズ（通常64）
        uint32 numberOfCores;       ///< 物理コア数
        uint32 numberOfThreads;     ///< 論理スレッド数（ハイパースレッディング含む）
    };

    /// プラットフォーム非依存のメモリ管理インターフェース
    ///
    /// ## スレッドセーフティ
    ///
    /// - **Init()**: メインスレッドから一度だけ呼び出す（起動時）
    /// - **GetStats()**: スレッドセーフ（内部で同期）
    /// - **GetConstants()**: スレッドセーフ（Init後は読み取り専用）
    /// - **BinnedAllocFromOS/BinnedFreeToOS**: スレッドセーフ（OS APIが保証）
    ///
    /// ## 初期化順序
    ///
    /// 1. エンジン起動時に `Init()` を呼び出す
    /// 2. `GetConstants()` は `Init()` 後のみ有効な値を返す
    /// 3. `Init()` 前に `GetConstants()` を呼ぶとゼロ初期化された値が返る
    struct GenericPlatformMemory
    {
        /// 初期化（起動時に一度だけ呼び出す）
        ///
        /// @note メインスレッドから呼び出すこと
        static void Init();

        /// 初期化済みかどうか
        static bool IsInitialized();

        /// メモリ統計取得
        ///
        /// @return 現在のメモリ使用状況（スナップショット）
        /// @note スレッドセーフ
        static PlatformMemoryStats GetStats();

        /// メモリ定数取得
        ///
        /// @return システムメモリ定数への参照
        /// @note Init()後は値が不変、スレッドセーフ
        /// @warning Init()前はゼロ初期化された値
        static const PlatformMemoryConstants& GetConstants();

        /// OS直接割り当て（大きなブロック用）
        ///
        /// @param size 割り当てサイズ（pageSize単位に切り上げ）
        /// @return 割り当てたメモリ、失敗時nullptr
        /// @note スレッドセーフ
        static void* BinnedAllocFromOS(SIZE_T size);

        /// OS直接解放
        ///
        /// @param ptr BinnedAllocFromOSで取得したポインタ
        /// @param size 割り当て時のサイズ
        /// @note スレッドセーフ、ptr==nullptrは無視
        static void BinnedFreeToOS(void* ptr, SIZE_T size);

        /// 仮想メモリ予約（コミットなし）
        ///
        /// @param size 予約サイズ
        /// @return 予約したアドレス、失敗時nullptr
        static void* VirtualReserve(SIZE_T size);

        /// 仮想メモリコミット
        ///
        /// @param ptr VirtualReserveで取得したアドレス
        /// @param size コミットサイズ
        /// @return 成功時true
        static bool VirtualCommit(void* ptr, SIZE_T size);

        /// 仮想メモリデコミット（予約は維持）
        static bool VirtualDecommit(void* ptr, SIZE_T size);

        /// 仮想メモリ解放（予約も解放）
        static void VirtualFree(void* ptr, SIZE_T size);

    private:
        static bool s_initialized;
        static PlatformMemoryConstants s_constants;
    };
}
```

## 検証

### テストファイル

`tests/engine/hal/MemoryTest.cpp` - `PlatformMemoryTest` セクション

### ユニットテスト

```cpp
TEST(PlatformMemory, Initialization)
{
    // Init前は未初期化
    // EXPECT_FALSE(NS::GenericPlatformMemory::IsInitialized());  // 初回のみ

    NS::GenericPlatformMemory::Init();
    EXPECT_TRUE(NS::GenericPlatformMemory::IsInitialized());

    // 二重Initは安全（何もしない）
    NS::GenericPlatformMemory::Init();
    EXPECT_TRUE(NS::GenericPlatformMemory::IsInitialized());
}

TEST(PlatformMemory, Constants)
{
    NS::GenericPlatformMemory::Init();
    const auto& c = NS::GenericPlatformMemory::GetConstants();

    EXPECT_GT(c.totalPhysical, 0);          // 物理メモリ > 0
    EXPECT_GT(c.totalVirtual, 0);           // 仮想メモリ > 0
    EXPECT_GE(c.pageSize, 4096);            // ページサイズ >= 4KB
    EXPECT_TRUE(NS::IsPowerOfTwo(c.pageSize));
    EXPECT_GE(c.allocationGranularity, c.pageSize);  // 粒度 >= ページ
    EXPECT_GT(c.cacheLineSize, 0);          // キャッシュライン > 0
    EXPECT_GT(c.numberOfCores, 0);          // コア数 > 0
    EXPECT_GE(c.numberOfThreads, c.numberOfCores);   // スレッド >= コア
}

TEST(PlatformMemory, Stats)
{
    NS::GenericPlatformMemory::Init();
    auto stats = NS::GenericPlatformMemory::GetStats();

    const auto& c = NS::GenericPlatformMemory::GetConstants();
    EXPECT_LE(stats.availablePhysical, c.totalPhysical);
    EXPECT_LE(stats.usedPhysical, c.totalPhysical);
    // availablePhysical + usedPhysical ≈ totalPhysical（厳密ではない）
}

TEST(PlatformMemory, BinnedAlloc)
{
    NS::GenericPlatformMemory::Init();
    const auto& c = NS::GenericPlatformMemory::GetConstants();

    // ページ単位で確保
    void* ptr = NS::GenericPlatformMemory::BinnedAllocFromOS(c.pageSize);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % c.pageSize, 0);  // ページ境界

    // 解放
    NS::GenericPlatformMemory::BinnedFreeToOS(ptr, c.pageSize);

    // nullptr解放は安全
    NS::GenericPlatformMemory::BinnedFreeToOS(nullptr, 0);
}

TEST(PlatformMemory, VirtualMemory)
{
    NS::GenericPlatformMemory::Init();
    const SIZE_T size = 1024 * 1024;  // 1MB

    // 予約のみ
    void* ptr = NS::GenericPlatformMemory::VirtualReserve(size);
    EXPECT_NE(ptr, nullptr);

    // コミット
    EXPECT_TRUE(NS::GenericPlatformMemory::VirtualCommit(ptr, size));

    // 使用可能
    static_cast<char*>(ptr)[0] = 'A';
    static_cast<char*>(ptr)[size - 1] = 'Z';

    // デコミット
    EXPECT_TRUE(NS::GenericPlatformMemory::VirtualDecommit(ptr, size));

    // 解放
    NS::GenericPlatformMemory::VirtualFree(ptr, size);
}
```

### エラーケース

| 操作 | 期待動作 | 回復 |
|------|----------|------|
| `BinnedAllocFromOS(巨大サイズ)` | nullptr返却 | 呼び出し側でOOM処理 |
| `VirtualCommit(無効ptr)` | false返却 | 再試行不可 |
| Init前の`GetConstants()` | ゼロ初期化値 | Initを先に呼ぶ |

## 注意事項

- **Init()前のGetConstants()は安全だがゼロ値を返す**
- GetStats()は毎回OSに問い合わせるため、頻繁な呼び出しは避ける
- VirtualReserve/Commitは大きなメモリプール管理に使用

## 次のサブ計画

→ 03-01b: Windowsメモリ実装

## 次のサブ計画

→ 03-01b: Windowsメモリ実装
