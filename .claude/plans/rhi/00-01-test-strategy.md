# 00-01: RHIテスト戦略

## 概要

RHI実装の品質保証戦略。ユニットテスト、統合テスト、モック戦略を定義。

## テスト階層

```
┌─────────────────────────────────────────────────────────────┐
│                   E2E Tests (手動)                         │
│ 実機GPU + 実際のレンダリング結果の目視確認                │
├─────────────────────────────────────────────────────────────┤
│               Integration Tests (CI)                        │
│ D3D12/Vulkan バックエンド + WARPアダプター                 │
├─────────────────────────────────────────────────────────────┤
│                 Unit Tests (CI)                            │
│ モックデバイス + インターフェース検証                      │
├─────────────────────────────────────────────────────────────┤
│                Static Analysis (CI)                        │
│ コンパイル検証 + 型安全性                                  │
└─────────────────────────────────────────────────────────────┘
```

## ユニットテスト戦略

### モッククラス

```cpp
// Source/Engine/RHI/Tests/Mocks/MockRHIDevice.h

namespace NS::RHI::Tests
{

/// モックリソース基底
class MockRHIResource : public IRHIResource
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
    void SetDebugName(const char* name) override { m_debugName = name; }
    const char* GetDebugName() const override { return m_debugName.c_str(); }

protected:
    std::atomic<uint32> m_refCount{1};
    std::string m_debugName;
};

/// モックバッファ
class MockRHIBuffer : public MockRHIResource, public IRHIBuffer
{
public:
    explicit MockRHIBuffer(const RHIBufferDesc& desc) : m_desc(desc) {}

    uint64 GetSize() const override { return m_desc.size; }
    ERHIBufferUsage GetUsage() const override { return m_desc.usage; }
    void* Map(uint64 offset, uint64 size) override
    {
        m_mapCount++;
        return m_data.data() + offset;
    }
    void Unmap() override { m_mapCount--; }

    // テスト用
    uint32 GetMapCount() const { return m_mapCount; }
    const std::vector<uint8>& GetData() const { return m_data; }

private:
    RHIBufferDesc m_desc;
    std::vector<uint8> m_data;
    uint32 m_mapCount = 0;
};

/// モックデバイス
class MockRHIDevice : public IRHIDevice
{
public:
    // ファクトリの呼び出し記録
    IRHIBuffer* CreateBuffer(const RHIBufferDesc& desc, const char* name) override
    {
        m_createBufferCalls.push_back(desc);
        return new MockRHIBuffer(desc);
    }

    // テスト用アクセサ
    const std::vector<RHIBufferDesc>& GetCreateBufferCalls() const
    {
        return m_createBufferCalls;
    }

    void ResetCallHistory()
    {
        m_createBufferCalls.clear();
    }

private:
    std::vector<RHIBufferDesc> m_createBufferCalls;
};

} // namespace NS::RHI::Tests
```

### テストケース例

```cpp
// Source/Engine/RHI/Tests/RefCountPtrTest.cpp

#include <gtest/gtest.h>
#include "RHI/Public/RHIRefCountPtr.h"
#include "Mocks/MockRHIDevice.h"

using namespace NS::RHI;
using namespace NS::RHI::Tests;

class RefCountPtrTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_device = std::make_unique<MockRHIDevice>();
    }

    std::unique_ptr<MockRHIDevice> m_device;
};

TEST_F(RefCountPtrTest, DefaultConstructor_CreatesNullPtr)
{
    TRefCountPtr<IRHIBuffer> ptr;

    EXPECT_FALSE(ptr.IsValid());
    EXPECT_EQ(ptr.Get(), nullptr);
}

TEST_F(RefCountPtrTest, ExplicitConstructor_IncrementsRefCount)
{
    auto* rawBuffer = m_device->CreateBuffer({ 1024 }, "Test");
    EXPECT_EQ(rawBuffer->GetRefCount(), 1);

    {
        TRefCountPtr<IRHIBuffer> ptr(rawBuffer);
        EXPECT_EQ(rawBuffer->GetRefCount(), 2);
    }

    EXPECT_EQ(rawBuffer->GetRefCount(), 1);
    rawBuffer->Release();
}

TEST_F(RefCountPtrTest, CopyConstructor_SharesOwnership)
{
    TRefCountPtr<IRHIBuffer> ptr1(m_device->CreateBuffer({ 1024 }, "Test"));
    ptr1.Get()->Release(); // Attach semantics

    {
        TRefCountPtr<IRHIBuffer> ptr2 = ptr1;
        EXPECT_EQ(ptr1.Get(), ptr2.Get());
        EXPECT_EQ(ptr1.GetRefCount(), 2);
    }

    EXPECT_EQ(ptr1.GetRefCount(), 1);
}

TEST_F(RefCountPtrTest, MoveConstructor_TransfersOwnership)
{
    TRefCountPtr<IRHIBuffer> ptr1(m_device->CreateBuffer({ 1024 }, "Test"));
    ptr1.Get()->Release();

    auto* rawPtr = ptr1.Get();
    TRefCountPtr<IRHIBuffer> ptr2 = std::move(ptr1);

    EXPECT_FALSE(ptr1.IsValid());
    EXPECT_EQ(ptr2.Get(), rawPtr);
    EXPECT_EQ(ptr2.GetRefCount(), 1);
}

TEST_F(RefCountPtrTest, Detach_DoesNotDecrement)
{
    TRefCountPtr<IRHIBuffer> ptr(m_device->CreateBuffer({ 1024 }, "Test"));
    ptr.Get()->Release();

    auto* rawPtr = ptr.Detach();

    EXPECT_FALSE(ptr.IsValid());
    EXPECT_EQ(rawPtr->GetRefCount(), 1);
    rawPtr->Release();
}

TEST_F(RefCountPtrTest, Reset_ReleasesOldAndAcquiresNew)
{
    auto* buffer1 = m_device->CreateBuffer({ 1024 }, "Buffer1");
    auto* buffer2 = m_device->CreateBuffer({ 2048 }, "Buffer2");

    TRefCountPtr<IRHIBuffer> ptr(buffer1);
    buffer1->Release();

    ptr.Reset(buffer2);
    buffer2->Release();

    EXPECT_EQ(ptr.Get(), buffer2);
    EXPECT_EQ(buffer2->GetRefCount(), 1);
    // buffer1 should be deleted
}
```

### フェンステスト例

```cpp
// Source/Engine/RHI/Tests/FenceTest.cpp

TEST_F(FenceTest, Signal_UpdatesCompletedValue)
{
    auto fence = m_device->CreateFence(0, "TestFence");
    TRefCountPtr<IRHIFence> ptr;
    ptr.Attach(fence);

    EXPECT_EQ(fence->GetCompletedValue(), 0);

    fence->Signal(5);

    EXPECT_EQ(fence->GetCompletedValue(), 5);
    EXPECT_TRUE(fence->IsCompleted(5));
    EXPECT_TRUE(fence->IsCompleted(3));
    EXPECT_FALSE(fence->IsCompleted(6));
}

TEST_F(FenceTest, Wait_ReturnsImmediatelyIfCompleted)
{
    auto fence = m_device->CreateFence(10, "TestFence");
    TRefCountPtr<IRHIFence> ptr;
    ptr.Attach(fence);

    auto start = std::chrono::high_resolution_clock::now();
    bool result = fence->Wait(5, 1000);
    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    EXPECT_TRUE(result);
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 10);
}

TEST_F(FenceTest, Wait_TimesOutIfNotCompleted)
{
    auto fence = m_device->CreateFence(0, "TestFence");
    TRefCountPtr<IRHIFence> ptr;
    ptr.Attach(fence);

    auto start = std::chrono::high_resolution_clock::now();
    bool result = fence->Wait(10, 50); // 50ms timeout
    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    EXPECT_FALSE(result);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 45);
}
```

### PSOテスト例

```cpp
// Source/Engine/RHI/Tests/PipelineStateTest.cpp

TEST_F(PipelineStateTest, CreateGraphicsPSO_WithValidShaders_Succeeds)
{
    auto vs = CreateMockVertexShader();
    auto ps = CreateMockPixelShader();

    RHIGraphicsPipelineStateDesc desc;
    desc.SetVS(vs.Get())
        .SetPS(ps.Get())
        .SetRootSignature(m_rootSig.Get())
        .SetRenderTargetFormats(RHIRenderTargetFormats::SingleRTWithDepth(
            EPixelFormat::RGBA8_UNORM));

    auto pso = m_device->CreateGraphicsPipelineState(desc, "TestPSO");

    EXPECT_NE(pso, nullptr);
    EXPECT_EQ(pso->GetVertexShader(), vs.Get());
    EXPECT_EQ(pso->GetPixelShader(), ps.Get());
}

TEST_F(PipelineStateTest, CreateGraphicsPSO_WithoutVS_Fails)
{
    RHIGraphicsPipelineStateDesc desc;
    desc.SetPS(CreateMockPixelShader().Get());

    auto pso = m_device->CreateGraphicsPipelineState(desc, "InvalidPSO");

    EXPECT_EQ(pso, nullptr);
    // 失敗原因は内部ログに出力される
}
```

## 統合テスト戦略

### WARPアダプター使用

```cpp
// Source/Engine/RHI/Tests/Integration/D3D12IntegrationTest.cpp

class D3D12IntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // WARPアダプターを強制使用（CI環境）
        RHIInitDesc initDesc;
        initDesc.preferredBackend = ERHIBackend::D3D12;
        initDesc.flags |= ERHIInitFlags::UseWARP;
        initDesc.flags |= ERHIInitFlags::EnableValidation;

        m_rhi = CreateDynamicRHI(initDesc);
        ASSERT_NE(m_rhi, nullptr);

        m_device = m_rhi->CreateDevice(m_rhi->GetAdapters()[0]);
        ASSERT_NE(m_device, nullptr);
    }

    void TearDown() override
    {
        m_device.Reset();
        m_rhi.Reset();
    }

    IDynamicRHIRef m_rhi;
    RHIDeviceRef m_device;
};

TEST_F(D3D12IntegrationTest, CreateBuffer_Upload_Read_Matches)
{
    // アップロードバッファ作成
    std::vector<uint32> srcData = { 1, 2, 3, 4, 5 };
    auto uploadBuffer = m_device->CreateBuffer({
        .size = srcData.size() * sizeof(uint32),
        .usage = ERHIBufferUsage::Upload,
    }, "UploadBuffer");

    // データ書き込み
    void* mapped = uploadBuffer->Map(0, 0);
    std::memcpy(mapped, srcData.data(), srcData.size() * sizeof(uint32));
    uploadBuffer->Unmap();

    // GPUバッファ作成
    auto gpuBuffer = m_device->CreateBuffer({
        .size = srcData.size() * sizeof(uint32),
        .usage = ERHIBufferUsage::Default,
    }, "GPUBuffer");

    // コピー実行
    auto context = m_device->GetImmediateContext();
    context->CopyBuffer(uploadBuffer.Get(), gpuBuffer.Get(), srcData.size() * sizeof(uint32));
    context->Flush();
    m_device->WaitIdle();

    // リードバック
    auto readbackBuffer = m_device->CreateBuffer({
        .size = srcData.size() * sizeof(uint32),
        .usage = ERHIBufferUsage::Readback,
    }, "ReadbackBuffer");

    context->CopyBuffer(gpuBuffer.Get(), readbackBuffer.Get(), srcData.size() * sizeof(uint32));
    context->Flush();
    m_device->WaitIdle();

    // 検証
    std::vector<uint32> dstData(srcData.size());
    void* readMapped = readbackBuffer->Map(0, 0);
    std::memcpy(dstData.data(), readMapped, dstData.size() * sizeof(uint32));
    readbackBuffer->Unmap();

    EXPECT_EQ(srcData, dstData);
}
```

## CI統合

```yaml
# .github/workflows/rhi-tests.yml

name: RHI Tests

on: [push, pull_request]

jobs:
  unit-tests:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - name: Build Tests
        run: |
          premake5 vs2022
          msbuild NS-ENGINE.sln /p:Configuration=Debug /p:Platform=x64

      - name: Run Unit Tests
        run: |
          ./Build/Debug/RHITests.exe --gtest_output=xml:test-results.xml

      - name: Upload Results
        uses: actions/upload-artifact@v4
        with:
          name: test-results
          path: test-results.xml

  integration-tests:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - name: Build Integration Tests
        run: |
          premake5 vs2022
          msbuild NS-ENGINE.sln /p:Configuration=Release /p:Platform=x64

      - name: Run Integration Tests (WARP)
        run: |
          ./Build/Release/RHIIntegrationTests.exe --gtest_filter="*WARP*"
```

## テストカバレッジ目標

| カテゴリ | 目標カバレッジ | 優先度 |
|----------|--------------|--------|
| TRefCountPtr | 95% | 高 |
| IRHIBuffer | 85% | 高 |
| IRHITexture | 85% | 高 |
| IRHIFence | 90% | 高 |
| IRHIPipelineState | 80% | 中 |
| IRHICommandContext | 75% | 中 |
| Transient系 | 70% | 中 |
| 統計・デバッグ | 60% | 低 |

## 検証

- [ ] モッククラスの完備
- [ ] ユニットテスト例の動作確認
- [ ] WARPでの統合テスト起動
- [ ] CI パイプライン構築
