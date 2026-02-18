# 20: ビュー作成

## 目的
SRV/UAV/CBV/RTV/DSVビューの作成。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md §Views
- source/engine/RHI/Public/IRHIViews.h

## TODO
- [ ] D3D12View.h/.cpp 作成
- [ ] SRV: CreateShaderResourceView（Buffer/Texture）
- [ ] UAV: CreateUnorderedAccessView + CBV: CreateConstantBufferView
- [ ] RTV: CreateRenderTargetView + DSV: CreateDepthStencilView
- [ ] IRHIShaderResourceView等のD3D12実装

## 完了条件
- 全ビュータイプ作成成功

## 見積もり
- 新規ファイル: 2 (D3D12View.h/.cpp)
