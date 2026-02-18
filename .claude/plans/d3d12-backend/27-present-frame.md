# 27: Present + フレームループ

## 目的
Presentとフレームループの統合。

## 参照
- docs/D3D12RHI/D3D12RHI_Part12_ViewportShaders.md §Present

## TODO
- [ ] Present(SyncInterval, Flags) + VSync/Tearing対応（VSync off+窓+Tearing対応時: DXGI_PRESENT_ALLOW_TEARING）
- [ ] フレーム完了フェンス: Present後にSignal + N-bufferingリソースリング
- [ ] BeginFrame → 描画 → EndFrame → Present ループ
- [ ] バックバッファ状態遷移: PRESENT → RENDER_TARGET → PRESENT
- [ ] Present失敗処理: リトライ（最大5回）、フルスクリーン喪失検出 + 復旧

## 制約
- Present失敗（E_INVALIDARG/DXGI_ERROR_INVALID_CALL）時は最大5回リトライ後abort
- フルスクリーン喪失: WS_VISIBLE検出 → ウィンドウモードフォールバック
- SyncInterval: VSync有効=1、無効=0

## 完了条件
- ウィンドウにクリアカラー表示、Present失敗時にクラッシュしない

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12SwapChain, D3D12CommandContext
