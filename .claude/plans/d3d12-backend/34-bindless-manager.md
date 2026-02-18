# 34: Bindless Manager

## 目的
Bindless描画モデルのディスクリプタ管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md §Bindless
- source/engine/RHI/Public/RHIBindless.h

## TODO
- [ ] グローバルSRV/UAVヒープの永続割り当て
- [ ] リソース作成時にBindlessハンドル発行
- [ ] ハンドル解放 → フリーリスト
- [ ] Unbounded descriptor range対応

## 完了条件
- Bindlessインデックスでリソースアクセス可能

## 見積もり
- 新規ファイル: 2 (D3D12Bindless.h/.cpp)
