# 17: Texture

## 目的
IRHITextureのD3D12実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md §Texture
- source/engine/RHI/Public/IRHITexture.h

## TODO
- [ ] D3D12Texture.h/.cpp + IRHITexture実装
- [ ] 2D/3D/Cubeテクスチャ作成（ミップレベル、配列テクスチャ対応）
- [ ] Depth/Stencil: PlaneCount=2対応（サブリソースインデックス = mip + arraySlice*mips + plane*mips*arrays）
- [ ] テクスチャアップロード: UpdateSubresources (upload heap経由)
- [ ] MSAAリゾルブ: Color→ResolveSubresource()（ハードウェア）、Depth/Stencil→シェーダーベースリゾルブ（ResolveDepthPS）、Typeless→UNORM変換

## 制約
- Depth/Stencilリソースは PlaneCount=2（Plane0=深度, Plane1=ステンシル）、サブリソース計算に必須
- 小テクスチャ4KBアライメント最適化: CanBe4KAligned条件（非MSAA、非Reserved、64KB以内）
- テクスチャ最大サイズ: 16384×16384×2048

## 完了条件
- テクスチャ作成+ミップデータアップロード成功、Depth/Stencilサブリソース正確

## 見積もり
- 新規ファイル: 2 (D3D12Texture.h/.cpp)
