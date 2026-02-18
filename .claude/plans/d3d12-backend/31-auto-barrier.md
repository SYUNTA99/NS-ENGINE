# 31: 自動バリア挿入 + StateTracker

## 目的
RHIResourceStateTrackerとD3D12バリアの接続、自動バリア挿入。

## 参照
- source/engine/RHI/Public/RHIStateTracking.h
- source/engine/RHI/Private/RHIStateTracking.cpp

## TODO
- [ ] RHIResourceStateTracker → D3D12バリア変換（RequireState() → Legacy/Enhanced分岐発行）
- [ ] 描画前: SRV→PIXEL_SHADER_RESOURCE, RT→RENDER_TARGET, DS→DEPTH_WRITE
- [ ] コピー前: COPY_SOURCE/COPY_DEST遷移、コピー後の元状態復帰
- [ ] レンダーパス: 開始時RT/DS遷移、終了時→次用途遷移、コマンドリスト完了時グローバル状態コミット
- [ ] バリアバッチ最適化: 同一リソース冗長バリア排除、逆遷移キャンセル

## 完了条件
- 三角形描画パスでバリア自動挿入動作、冗長バリア排除確認

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Barriers, D3D12CommandContext
