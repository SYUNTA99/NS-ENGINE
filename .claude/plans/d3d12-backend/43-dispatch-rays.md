# 43: SBT + DispatchRays

## 目的
シェーダーバインディングテーブルとDispatchRays実行。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §SBT, §Dispatch
- source/engine/RHI/Public/RHIRaytracingShader.h

## TODO
- [ ] SBTレイアウト構築: ShaderID(32B) + ローカルRSパラメータ（Bindless/Traditional切替下記参照）
- [ ] D3D12_DISPATCH_RAYS_DESC構築
- [ ] リソースバインディング（グローバル + ローカル）
- [ ] ID3D12GraphicsCommandList4::DispatchRays()
- [ ] DispatchTable登録 + HasRayTracingSupport() = true

## 制約
- SBTレコード構造（モード別）:
  - Bindless: ShaderID(32B) + ハンドルインデックス（小サイズ）
  - Traditional: ShaderID(32B) + GPU仮想アドレス直接格納（大サイズ）
- レコードストライド: D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT(32B)アライメント必須
- シェーダー識別子検証: 全ビット~0 = invalidの場合はDispatch前にエラー
- ローカルRS最大サイズ: 4KB/レコード

## 完了条件
- DispatchRaysで簡単なレイトレーシング描画、Bindless/Traditional両対応

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingSBT.h/.cpp)
