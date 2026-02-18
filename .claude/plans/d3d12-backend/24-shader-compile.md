# 24: シェーダーコンパイル

## 目的
HLSL→DXILコンパイルとIRHIShader実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part08_PipelineState.md §Shader
- source/engine/RHI/Public/IRHIShader.h

## TODO
- [ ] D3D12Shader.h/.cpp + IRHIShader実装（VS/PS/CS/GS/HS/DS）
- [ ] HLSL→DXILコンパイル（IDxcCompiler3）
- [ ] シェーダーリフレクション: 入力レイアウト自動抽出
- [ ] プリコンパイル済みシェーダー(.cso)読み込み

## 完了条件
- HLSLシェーダーコンパイル→バイトコード取得成功

## 見積もり
- 新規ファイル: 2 (D3D12Shader.h/.cpp)
