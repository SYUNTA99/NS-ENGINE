# 37: Occlusion + GPU Profiler

## 目的
オクルージョンクエリとGPUプロファイラー。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md
- source/engine/RHI/Public/RHIOcclusion.h
- source/engine/RHI/Public/RHIGPUProfiler.h

## TODO
- [ ] Predicated Rendering対応
- [ ] Binary / Precise occlusion
- [ ] フレーム全体のGPU時間計測
- [ ] ドローコール単位計測
- [ ] PIX連携（オプション）

## 完了条件
- オクルージョンクエリ動作、フレームGPU時間計測

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Query, D3D12CommandContext
