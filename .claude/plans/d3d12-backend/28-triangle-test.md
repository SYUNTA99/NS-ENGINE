# 28: 三角形描画テスト

## 目的
Phase 1の集大成: 画面に三角形を描画する。

## 参照
- Phase 1 全サブ計画の統合

## TODO
- [ ] 最小頂点シェーダー（position passthrough）+ ピクセルシェーダー（solid color）
- [ ] 頂点バッファ: 3頂点作成 + データアップロード
- [ ] PSO作成: VS + PS + InputLayout
- [ ] DrawInstanced(3, 1, 0, 0) 実行
- [ ] 画面にカラー三角形表示確認

## GPU検証チェックリスト
- D3D12 Debug Layer警告/エラー: 0件（ID3D12InfoQueue メッセージ確認）
- PIX/RenderDocキャプチャ: DrawCall=1、正しいVB/IB/PSO/RS バインド確認
- リソースバリア遷移: PRESENT→RENDER_TARGET→PRESENT が正確（余分な遷移なし）
- ディスクリプタヒープ: リーク検出（フレーム間でヒープ使用量が単調増加しないこと）
- DeferredDelete: Shutdown時に全リソース解放完了（リーク0）

## 完了条件
- カラー三角形が画面に描画される、4構成ビルド通過、GPU検証チェックリスト全項目クリア

## 見積もり
- 新規ファイル: 2 (テストシェーダー .hlsl × 2)
- 変更ファイル: フレームループ統合
