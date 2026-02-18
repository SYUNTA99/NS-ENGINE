# 18: Upload Heap + フォーマット変換

## 目的
初期データ転送用Upload Heapとピクセルフォーマット変換テーブル。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md §Upload
- docs/D3D12RHI/D3D12RHI_Part05_MemoryAllocation.md §Upload

## TODO
- [ ] 一時アップロードバッファ作成
- [ ] CopyBufferRegion / CopyTextureRegion
- [ ] フレーム完了後の一時バッファ解放
- [ ] ERHIPixelFormat ↔ DXGI_FORMAT変換テーブル（Typeless/SRV/UAV/RTV/DSVフォーマット派生含む）
- [ ] サーフェスリードバック: READBACKステージングバッファ作成 → CopyTextureRegion → GPU同期(SubmitAndBlockUntilGPUIdle) → Map/Unmap読取

## 制約
- リードバック手順（必須順序）: READBACK heap作成 → ソースをCopySrcバリア → CopyTextureRegion → 元状態復帰バリア → GPU同期 → Map → 読取 → Unmap
- 3Dテクスチャ深度スライスストライド: Align(W × BytesPerPixel, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) × H

## 完了条件
- バッファ/テクスチャへの初期データ転送成功、テクスチャリードバック動作

## 見積もり
- 新規ファイル: 2 (D3D12Upload.h/.cpp)
