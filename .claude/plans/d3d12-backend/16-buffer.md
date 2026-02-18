# 16: Buffer

## 目的
IRHIBufferのD3D12実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md §Buffer
- source/engine/RHI/Public/IRHIBuffer.h

## TODO
- [ ] D3D12Buffer.h/.cpp + IRHIBuffer実装
- [ ] CreateCommittedResourceでバッファ作成（アライメント規則下記参照）
- [ ] ヒープタイプ判定: DEFAULT / UPLOAD / READBACK
- [ ] 頂点/インデックス/定数/構造化/間接引数バッファ対応
- [ ] D3D12_RESOURCE_FLAG設定: UAV→ALLOW_UNORDERED_ACCESS、非SRV→DENY_SHADER_RESOURCE

## 制約
- バッファアライメント: Raw/Typedビュー=16B最小、Structured=LCM(Stride,16)、Readback=512B
- 定数バッファ: 256Bアライメント（D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT）
- Placed Resource最小サイズ: 64KB

## 完了条件
- 各種バッファ作成成功、GPUVirtualAddress取得可能、アライメント正確

## 見積もり
- 新規ファイル: 2 (D3D12Buffer.h/.cpp)
