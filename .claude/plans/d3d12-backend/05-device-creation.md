# 05: Device作成 + デバッグ支援

## 目的
ID3D12Deviceを作成し、デバッグレイヤーとFeature検出を設定する。

## 参照
- docs/D3D12RHI/D3D12RHI_Part02_AdapterDeviceQueue.md
- source/engine/RHI/Public/IRHIDevice.h

## TODO
- [x] D3D12Device.h/.cpp — D3D12CreateDevice + IRHIDevice実装 + ID3D12CommandSignature作成（DrawIndirect/DispatchIndirect用、デバイスレベルでキャッシュ）
- [x] CheckFeatureSupport (Options, Options1, Options5, Options7, Options12, Options16, Options21)
- [x] Debug Layer: ID3D12Debug::EnableDebugLayer + GPU Based Validation
- [x] DRED設定 + ID3D12InfoQueue メッセージフィルタ
- [x] HRESULTエラーハンドリング（LOG_HRESULT マクロ活用）
- [x] RHIヘッダー依存修正（IRHIDevice.h, RHIGPUEvent.h, RHIRaytracingAS.h, RHIFwd.h, RHIEnums.h）

## 完了条件
- D3D12デバイス作成成功（Debug Layer有効）、CommandSignatureキャッシュ済み

## 見積もり
- 新規ファイル: 2 (D3D12Device.h/.cpp)
