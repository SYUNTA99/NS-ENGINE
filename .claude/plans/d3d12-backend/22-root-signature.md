# 22: RootSignature

## 目的
D3D12のRoot Signature作成とIRHIRootSignature実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part08_PipelineState.md §RootSignature
- source/engine/RHI/Public/IRHIRootSignature.h

## TODO
- [ ] D3D12RootSignature.h/.cpp + IRHIRootSignature実装
- [ ] シリアライズ → CreateRootSignature
- [ ] テーブルレイアウト: CBV/SRV/UAV/Samplerスロット配置（64 DWORD予算管理）
- [ ] Root Descriptor（インラインCBV）対応
- [ ] 静的サンプラー定義

## 制約
- **64 DWORDハードリミット**: Root Signatureパラメータ合計が64 DWORDを超えるとCreateRootSignature失敗
- DWORDコスト: Descriptor Table=1, Root CBV/SRV/UAV=2, Root Constants=N(個数)
- パラメータ配置優先度: PS → VS → GS → MS → AS → All、各ステージ内: SRV → CBV → Sampler → UAV
- パフォーマンス警告: ラスタRS 12 DWORD超過時

## 完了条件
- Root Signature作成成功、テーブルレイアウト設定可能、64 DWORD以内

## 見積もり
- 新規ファイル: 2 (D3D12RootSignature.h/.cpp)
