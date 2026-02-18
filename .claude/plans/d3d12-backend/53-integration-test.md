# 53: 統合テスト

## 目的
全フェーズの統合検証。

## 参照
- 全サブ計画

## TODO
- [ ] Phase 1テスト: 三角形描画E2E（デバイス初期化→Present）
- [ ] Phase 2テスト: バリア自動挿入 + Bindlessバインディング + GPU Timestamp
- [ ] Phase 3テスト: DXR DispatchRays + Mesh DispatchMesh
- [ ] Shipping構成: NS_RHI_STATIC_BACKEND=D3D12 ゼロオーバーヘッド
- [ ] 全4構成最終ビルド確認

## フェーズ別合格基準
- Phase 1: D3D12 Debug Layer警告0件、三角形描画E2Eで100フレーム安定動作、DeferredDeleteリーク0
- Phase 2: バリア自動挿入でD3D12検証レイヤー警告0件、GPUタイムスタンプ取得成功（Direct/Copyキュー両方）、DeviceLost検出→エラー報告パス確認
- Phase 3: DXR/Mesh/WG Feature未対応GPU→スキップ判定（HasXxxSupport()=false時にクラッシュしない）、対応GPU→DispatchRays/DispatchMesh実行成功
- Shipping: NS_RHI_DISPATCH マクロ展開が直接呼出しになっていることをobjdump/逆アセンブリで確認

## 完了条件
- 全フェーズ合格基準クリア、全4構成ビルド通過

## 見積もり
- 新規ファイル: テストコード
