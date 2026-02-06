# 05-02: PlatformTLS

## 目的

スレッドローカルストレージ（TLS）の抽象化を実装する。

## 参考

[Platform_Abstraction_Layer_Part5.md](docs/Platform_Abstraction_Layer_Part5.md) - セクション2「スレッドローカルストレージ」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

- 02-03 関数呼び出し規約（`FORCEINLINE` ← `NS_FORCEINLINE`のエイリアス）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformTLS.h`
- `source/engine/hal/Public/Windows/WindowsPlatformTLS.h`
- `source/engine/hal/Public/HAL/PlatformTLS.h`

## TODO

- [ ] `GenericPlatformTLS` 構造体（TLS操作インターフェース）
- [ ] `WindowsPlatformTLS` 構造体（TlsAlloc/TlsFree使用）
- [ ] `PlatformTLS.h` エントリポイント
- [ ] 無効スロット定数定義

## 実装内容

```cpp
// GenericPlatformTLS.h
namespace NS
{
    struct GenericPlatformTLS
    {
        static constexpr uint32 kInvalidTlsSlot = 0xFFFFFFFF;

        static uint32 AllocTlsSlot();
        static void FreeTlsSlot(uint32 slotIndex);
        static void SetTlsValue(uint32 slotIndex, void* value);
        static void* GetTlsValue(uint32 slotIndex);
    };
}

// WindowsPlatformTLS.h
namespace NS
{
    struct WindowsPlatformTLS : public GenericPlatformTLS
    {
        static FORCEINLINE uint32 AllocTlsSlot()
        {
            return ::TlsAlloc();
        }

        static FORCEINLINE void FreeTlsSlot(uint32 slotIndex)
        {
            ::TlsFree(slotIndex);
        }

        static FORCEINLINE void SetTlsValue(uint32 slotIndex, void* value)
        {
            ::TlsSetValue(slotIndex, value);
        }

        static FORCEINLINE void* GetTlsValue(uint32 slotIndex)
        {
            return ::TlsGetValue(slotIndex);
        }
    };

    typedef WindowsPlatformTLS PlatformTLS;
}
```

## 検証

- TLSスロット割り当て/解放が正常動作
- スレッドごとに独立した値を保持
