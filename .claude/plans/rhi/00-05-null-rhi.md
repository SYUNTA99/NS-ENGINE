# 00-05: NullRHIバックエンド

## 目的

GPU不要のCI/テスト用Nullバックエンド。全操作が即座に成功を返す。

## ファイル

新規作成:
- `Source/Engine/NullRHI/Private/NullRHI.h`
- `Source/Engine/NullRHI/Private/NullRHI.cpp`

## 実装方針

- IDynamicRHIを継承、全ファクトリメソッドがスタブリソースを返す
- NullRHIBuffer/NullRHITexture: メモリ割り当てなし、サイズ/フォーマット情報のみ保持
- NullRHICommandContext: 全コマンドがno-op
- NullRHIFence: Signal即完了
- NullRHISwapChain: Present即return

## スタブリソース設計

```cpp
namespace NS::RHI::Private
{
    /// NullRHI共通リソース基底
    /// IRHIResourceの必須メソッドをスタブ実装
    class NullRHIResourceBase : public IRHIResource
    {
    public:
        uint32 AddRef() override { return ++m_refCount; }
        uint32 Release() override
        {
            uint32 count = --m_refCount;
            if (count == 0) delete this;
            return count;
        }
        uint32 GetRefCount() const override { return m_refCount; }
        void SetDebugName(const char* name) override { /* no-op */ }
        const char* GetDebugName() const override { return "NullRHI"; }

    protected:
        std::atomic<uint32> m_refCount{1};
        virtual ~NullRHIResourceBase() = default;
    };

    /// Nullバッファ: メモリ割り当てなし
    /// @note IRHIBufferがIRHIResourceを仮想継承している場合はNullRHIResourceBaseのみで良い。
    ///       実装時にIRHIBufferの継承関係に応じて調整。
    class NullRHIBuffer final : public NullRHIResourceBase, public IRHIBuffer
    {
    public:
        NS_DISALLOW_COPY_AND_MOVE(NullRHIBuffer);

        explicit NullRHIBuffer(const RHIBufferDesc& desc)
            : m_desc(desc) {}

        // IRHIBuffer実装: サイズ/フラグ情報のみ返す
        uint64 GetSize() const override { return m_desc.size; }
        void* Map() override { return nullptr; }
        void Unmap() override {}

    private:
        RHIBufferDesc m_desc;
    };

    /// Nullテクスチャ: フォーマット情報のみ保持
    class NullRHITexture final : public NullRHIResourceBase, public IRHITexture
    {
    public:
        NS_DISALLOW_COPY_AND_MOVE(NullRHITexture);

        explicit NullRHITexture(const RHITextureDesc& desc)
            : m_desc(desc) {}

    private:
        RHITextureDesc m_desc;
    };

    /// Nullフェンス: Signal即完了
    class NullRHIFence final : public NullRHIResourceBase, public IRHIFence
    {
    public:
        NS_DISALLOW_COPY_AND_MOVE(NullRHIFence);

        void Signal(uint64 value) override { m_value = value; }
        uint64 GetCompletedValue() const override { return m_value; }
        bool Wait(uint64 value, uint64 /*timeoutMs*/) override { return true; }

    private:
        uint64 m_value = 0;
    };
}
```

## 使用場面

- CIユニットテスト (00-01-test-strategy.md)
- ヘッドレスサーバー
- インターフェース互換性検証

## 依存

- Phase 1完了後に実装可能
- IDynamicRHI (01-15) + 全リソースインターフェース

## premake5.lua

```lua
project "NullRHI"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    files {
        "Source/Engine/NullRHI/Public/**.h",
        "Source/Engine/NullRHI/Private/**.h",
        "Source/Engine/NullRHI/Private/**.cpp",
    }
    includedirs {
        "Source/Engine/NullRHI/Public",
        "Source/Engine/NullRHI/Private",
        "Source/Engine/RHI/Public",
        "Source/Common",
    }
    links { "RHI", "Common" }
```
