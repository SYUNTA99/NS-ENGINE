# 35: ResourceTable + Bindless RootSig

## 目的
リソーステーブルのD3D12実装とBindless版Root Signature。

## 参照
- source/engine/RHI/Public/RHIResourceTable.h

## TODO
- [ ] リソーステーブルD3D12実装
- [ ] 動的追加/削除
- [ ] テーブルインデックスの定数バッファ経由渡し
- [ ] SM6.6 ResourceDescriptorHeap / SamplerDescriptorHeap

## 完了条件
- 動的リソース追加/削除、Bindless vs Table-based切替

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Bindless, D3D12RootSignature
