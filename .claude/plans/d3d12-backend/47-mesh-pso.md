# 47: Mesh Shader PSO

## 目的
Mesh/Amplification ShaderパイプラインのPSO作成。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §MeshShader
- source/engine/RHI/Public/RHIMeshPipelineState.h

## TODO
- [ ] D3D12_MESH_SHADER_PIPELINE_STATE_DESC構築
- [ ] Amplification Shader (AS) + Mesh Shader (MS) + Pixel Shader (PS)
- [ ] DXCコンパイル: -T as_6_5 / -T ms_6_5
- [ ] RHIMeshPipelineState/RHIMeshShader D3D12実装
- [ ] Feature検出: D3D12_FEATURE_D3D12_OPTIONS7 (MeshShaderTier)

## 完了条件
- Mesh Shader PSO作成成功

## 見積もり
- 新規ファイル: 2 (D3D12MeshShader.h/.cpp)
