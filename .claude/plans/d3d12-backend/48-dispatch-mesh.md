# 48: DispatchMesh

## 目的
DispatchMesh実行とAS→MSペイロード渡し。

## 参照
- source/engine/RHI/Public/RHIMeshDispatch.h

## TODO
- [ ] ID3D12GraphicsCommandList6::DispatchMesh(X, Y, Z)
- [ ] ExecuteIndirect + D3D12_DISPATCH_MESH_ARGUMENTS
- [ ] ASからMSへのペイロード渡し
- [ ] GPU-drivenメッシュレットカリング対応
- [ ] DispatchTable登録 + HasMeshShaderSupport() = true

## 完了条件
- DispatchMesh動作、AS+MS二段パイプライン動作

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12MeshShader, D3D12CommandContext
