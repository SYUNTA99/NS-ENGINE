# 03-02: PlatformAtomics（統合計画 → 細分化済み）

> **注意**: この計画は細分化されました。以下を参照してください：
> - [03-02a: アトミックインターフェース](03-02a-atomics-interface.md)
> - [03-02b: Windowsアトミック実装](03-02b-windows-atomics.md)

---

# 03-02: PlatformAtomics（旧）

## 目的

アトミック操作の抽象化レイヤーを実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション4「アトミック操作」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`int32`, `int64`）

## 依存（HAL）

- 02-03 関数呼び出し規約（`FORCEINLINE` ← `NS_FORCEINLINE`のエイリアス）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformAtomics.h`
- `source/engine/hal/Public/Windows/WindowsPlatformAtomics.h`
- `source/engine/hal/Public/HAL/PlatformAtomics.h`

## TODO

- [ ] `GenericPlatformAtomics` 構造体（インターフェース定義）
- [ ] `WindowsPlatformAtomics` 構造体（Interlocked系API使用）
- [ ] `PlatformAtomics.h` エントリポイント
- [ ] 128ビットアトミック対応（条件付き）

## 実装内容

```cpp
// GenericPlatformAtomics.h
namespace NS
{
    struct GenericPlatformAtomics
    {
        static FORCEINLINE int32 InterlockedIncrement(volatile int32* value);
        static FORCEINLINE int32 InterlockedDecrement(volatile int32* value);
        static FORCEINLINE int32 InterlockedAdd(volatile int32* value, int32 amount);
        static FORCEINLINE int32 InterlockedExchange(volatile int32* value, int32 exchange);
        static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* dest, int32 exchange, int32 comparand);

        static FORCEINLINE int64 InterlockedIncrement(volatile int64* value);
        static FORCEINLINE int64 InterlockedDecrement(volatile int64* value);
        static FORCEINLINE int64 InterlockedAdd(volatile int64* value, int64 amount);
        static FORCEINLINE int64 InterlockedExchange(volatile int64* value, int64 exchange);
        static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* dest, int64 exchange, int64 comparand);

        static FORCEINLINE void* InterlockedExchangePtr(void** dest, void* exchange);
        static FORCEINLINE void* InterlockedCompareExchangePtr(void** dest, void* exchange, void* comparand);
    };
}

// WindowsPlatformAtomics.h
namespace NS
{
    struct WindowsPlatformAtomics : public GenericPlatformAtomics
    {
        static FORCEINLINE int32 InterlockedIncrement(volatile int32* value)
        {
            return (int32)::_InterlockedIncrement((volatile long*)value);
        }
        // ... 他のメソッド
    };

    typedef WindowsPlatformAtomics PlatformAtomics;
}
```

## 検証

- マルチスレッド環境でカウンタが正しく増減
- CompareExchange が期待通り動作
- ポインタのアトミック操作が64bit環境で正常動作

## 必要なヘッダ（Windows実装）

- `<intrin.h>` - `_InterlockedIncrement`, `_InterlockedDecrement`, `_InterlockedCompareExchange`

## 実装詳細

### WindowsPlatformAtomics.h（インライン実装）

```cpp
static FORCEINLINE int32 InterlockedIncrement(volatile int32* value)
{
    return (int32)::_InterlockedIncrement((volatile long*)value);
}

static FORCEINLINE int64 InterlockedIncrement(volatile int64* value)
{
    return (int64)::_InterlockedIncrement64(value);
}

static FORCEINLINE void* InterlockedCompareExchangePtr(void** dest, void* exchange, void* comparand)
{
    return ::_InterlockedCompareExchangePointer(dest, exchange, comparand);
}
```

## 注意事項

- Windowsでは `long` は32bit（`int32` と同サイズ）
- 64bit版関数は `_Interlocked*64` サフィックス
- ポインタ版は `_InterlockedCompareExchangePointer` を使用
- すべてFORCEINLINEでヘッダに実装（ゼロオーバーヘッド）
