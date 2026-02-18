# 04: DXGI Factory + Adapter列挙

## 目的
IDXGIFactory6でGPUアダプタを列挙しIRHIAdapterを実装する。

## 参照
- docs/D3D12RHI/D3D12RHI_Part02_AdapterDeviceQueue.md
- source/engine/RHI/Public/IRHIAdapter.h

## TODO
- [ ] D3D12Adapter.h/.cpp 作成
- [ ] IDXGIFactory6::EnumAdapterByGpuPreference
- [ ] DXGI_ADAPTER_DESC3 → RHIAdapterDesc変換
- [ ] D3D_FEATURE_LEVEL_12_0以上の判定
- [ ] IRHIAdapter実装: D3D12Adapter : public IRHIAdapter

## 完了条件
- Adapter情報がログに出力される

## 見積もり
- 新規ファイル: 2 (D3D12Adapter.h/.cpp)
