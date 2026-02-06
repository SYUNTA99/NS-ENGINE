# 03-01: PlatformMemory（統合計画 → 細分化済み）

> **注意**: この計画は細分化されました。以下を参照してください：
> - [03-01a: メモリ型定義](03-01a-memory-types.md)
> - [03-01b: Windowsメモリ実装](03-01b-windows-memory.md)

---

# 03-01: PlatformMemory（旧）

## 目的

メモリ管理の抽象化レイヤーを実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション1「メモリ管理」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`uint64`）

## 依存（HAL）

- 01-04 プラットフォーム型（`SIZE_T`）
- 02-03 関数呼び出し規約（`FORCEINLINE` ← `NS_FORCEINLINE`のエイリアス）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformMemory.h`
- `source/engine/hal/Public/Windows/WindowsPlatformMemory.h`
- `source/engine/hal/Public/HAL/PlatformMemory.h`
- `source/engine/hal/Private/Windows/WindowsPlatformMemory.cpp`

## TODO

- [ ] `GenericPlatformMemory` 構造体（メモリ統計、定数）
- [ ] `WindowsPlatformMemory` 構造体（VirtualAlloc/VirtualFree）
- [ ] `PlatformMemory.h` エントリポイント
- [ ] メモリ統計構造体（PlatformMemoryStats, PlatformMemoryConstants）

## 実装内容

```cpp
// GenericPlatformMemory.h
namespace NS
{
    struct PlatformMemoryStats
    {
        uint64 availablePhysical;
        uint64 availableVirtual;
        uint64 usedPhysical;
        uint64 usedVirtual;
    };

    struct PlatformMemoryConstants
    {
        uint64 totalPhysical;
        uint64 totalVirtual;
        SIZE_T pageSize;
        SIZE_T allocationGranularity;
    };

    struct GenericPlatformMemory
    {
        static void Init();
        static PlatformMemoryStats GetStats();
        static const PlatformMemoryConstants& GetConstants();
        static void* BinnedAllocFromOS(SIZE_T size);
        static void BinnedFreeToOS(void* ptr, SIZE_T size);
    };
}
```

## 検証

- GetStats() がシステムメモリ情報を返す
- BinnedAllocFromOS/BinnedFreeToOS が正常動作
- GetConstants() の pageSize が 4096（典型的なWindowsページサイズ）

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `VirtualAlloc`, `VirtualFree`, `GlobalMemoryStatusEx`
- `<sysinfoapi.h>` - `GetSystemInfo`

## 実装詳細

### WindowsPlatformMemory.cpp

```cpp
void* WindowsPlatformMemory::BinnedAllocFromOS(SIZE_T size)
{
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void WindowsPlatformMemory::BinnedFreeToOS(void* ptr, SIZE_T size)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

PlatformMemoryStats WindowsPlatformMemory::GetStats()
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);

    PlatformMemoryStats stats;
    stats.availablePhysical = statex.ullAvailPhys;
    stats.availableVirtual = statex.ullAvailVirtual;
    stats.usedPhysical = statex.ullTotalPhys - statex.ullAvailPhys;
    stats.usedVirtual = statex.ullTotalVirtual - statex.ullAvailVirtual;
    return stats;
}
```

## 注意事項

- `BinnedAllocFromOS` はアロケータの最下層として使用
- ページサイズ境界でのみ割り当て可能
- 大量のメモリ確保はOSへの直接コール（オーバーヘッドあり）
