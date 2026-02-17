# Findings: RHI Zero-Abstraction Refactoring

## 要件

**目標**: 「仮想関数呼び出しをフレームあたり数回に減らす設計」へ移行
- virtual OK: `Begin()`, `Finish()`, `Submit()`, デバイス/リソース生成（COLD path）
- virtual NG: Draw/Dispatch/Barrier/Bind等のホットパス（1フレーム数百〜数千回）
- コマンド記録: POD構造体 → リニアアロケータ（malloc禁止）
- コマンド実行: DispatchTable（開発）/ 条件コンパイル直接呼出（出荷）

## 現状分析

### 2つの並存経路（問題）

| 経路 | 仕組み | virtual回数/フレーム |
|------|--------|---------------------|
| A: IRHI* virtual | ctx->Draw() → vtable | 数百〜数千 |
| B: CommandBuffer + DispatchTable | CmdDraw → switch → NS_RHI_DISPATCH | 0（switchのみ） |

**経路Bが設計意図通り。経路AのIRHI* virtualホットパスを排除する。**

### Virtual関数のインベントリ

| インターフェース | virtual数 | 温度 | 方針 |
|----------------|----------|------|------|
| IRHICommandContextBase | 20 | HOT（barrier/copy） | **virtual排除** → DispatchTable |
| IRHIComputeContext | 19 | HOT（dispatch/bind） | **virtual排除** → DispatchTable |
| IRHICommandContext | 52 | HOT（draw/bind） | **virtual排除** → DispatchTable |
| IRHIImmediateContext | 2+20継承 | WARM | Begin/Finish/FlushだけvirtualでOK |
| IRHIUploadContext | 0 | - | **既にDispatchTable使用（模範）** |
| IRHIDevice | 143 | COLD（生成/クエリ） | **virtual維持** |
| IRHIQueue | 28 | WARM（Submit/Fence） | **virtual維持**（フレームあたり数回） |
| IRHIAdapter | 18 | COLD | **virtual維持** |
| IDynamicRHI | ~50 | COLD | **virtual維持** |
| IRHIBuffer | 10 | Mixed | GetGPUVirtualAddress()だけHOT→キャッシュ |
| IRHITexture | 18 | Mixed | ほぼCOLD |
| IRHIViews (SRV/UAV/CBV) | 7-10各 | HOT (handle取得) | **handle値をキャッシュ** |
| IRHIResource | 7 | COLD | **virtual維持** |
| IRHIPipelineState系 | 5-6 | COLD | **virtual維持** |

### バックエンド状況

- **バックエンド実装なし**（D3D12/Vulkan未実装）
- Legacy DX11は別モジュール（RHI経由しない）
- GRHIDispatchTable はゼロ初期化のまま
- NS_BUILD_SHIPPING / NS_RHI_STATIC_BACKEND は未定義（dead code）
- 誰もRHIを使っていない → **破壊的変更のコスト = ゼロ**

### 既に正しいパターン（模範）

**IRHIUploadContext** (`IRHIUploadContext.h`):
```cpp
class IRHIUploadContext : public IRHICommandContextBase {
    void UploadBuffer(...) {
        NS_RHI_DISPATCH(UploadBuffer, this, dst, dstOffset, srcData, srcSize);
    }
};
```
→ virtualなし、DispatchTable直接。これを全contextに拡大する。

**RHICommandBuffer** (`RHICommandBuffer.h`):
- AllocCommand<CmdDraw>() でPOD記録
- Execute() でswitch → CmdXxx::Execute() → NS_RHI_DISPATCH()
- リニアアロケータ、フレーム毎リセット

## 設計決定

| 決定 | 理由 |
|------|------|
| Context virtualをDispatchTable inline化 | IRHIUploadContext が既に模範。統一する |
| IDynamicRHI/IRHIDevice/IRHIQueueはvirtual維持 | COLD path。フレームあたり数回。抽象化コストは無視できる |
| View handle値をPODにキャッシュ | GetCPUHandle()/GetGPUHandle()のvirtual排除 |
| IRHIBuffer::GetGPUVirtualAddress()をメンバ変数化 | HOT path唯一のvirtual。作成時にキャッシュ |
| RHICommandBufferをプライマリAPI化 | 既存実装を活用。新規コードはCmdXxx経由で記録 |

## 重要ファイル

| ファイル | 役割 |
|---------|------|
| `source/engine/RHI/Public/IRHICommandContextBase.h` | Context基底（20 virtual → inline化） |
| `source/engine/RHI/Public/IRHIComputeContext.h` | Compute context（19 virtual → inline化） |
| `source/engine/RHI/Public/IRHICommandContext.h` | Graphics context（52 virtual → inline化） |
| `source/engine/RHI/Public/IRHIUploadContext.h` | Upload context（模範実装） |
| `source/engine/RHI/Public/RHIDispatchTable.h` | DispatchTable + マクロ |
| `source/engine/RHI/Public/RHICommands.h` | コマンド構造体 |
| `source/engine/RHI/Internal/RHICommandBuffer.h` | コマンドバッファ |
| `source/engine/RHI/Public/IRHIViews.h` | View interfaces（handle HOT path） |
| `source/engine/RHI/Public/IRHIBuffer.h` | Buffer（GetGPUVirtualAddress HOT） |
| `source/engine/RHI/Public/IRHIDevice.h` | Device（143 virtual, COLD維持） |
| `source/engine/RHI/Public/IDynamicRHI.h` | Factory（virtual維持） |
