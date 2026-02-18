# 52: DispatchTable検証

## 目的
全構成でのDispatchTable動作検証。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h

## TODO
- [ ] Dev構成: GRHIDispatchTable経由の関数ポインタ呼出し検証
- [ ] Shipping構成: NS_RHI_EXPAND経由の直接呼出し検証
- [ ] CommandBuffer Executeパスの確認
- [ ] 4構成ビルド通過（Debug/Release/Burst/Shipping）
- [ ] パフォーマンス比較: Dev(間接) vs Shipping(直接)

## 完了条件
- 全構成動作、ゼロオーバーヘッド確認

## 見積もり
- 新規ファイル: 0
- 変更ファイル: テスト/検証コード
