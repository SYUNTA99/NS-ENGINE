# 09: Submission

## 目的
コマンドリストのキューへの投入とフレーム完了同期。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md §Submission

## TODO
- [ ] D3D12Submission.h/.cpp 作成
- [ ] コマンドリストのキューへの投入
- [ ] バッチ投入（複数CommandList一括実行）
- [ ] フレーム完了同期: Present後のフェンス待機

## 完了条件
- コマンドリスト投入→GPU完了待機の一連動作

## 見積もり
- 新規ファイル: 2 (D3D12Submission.h/.cpp)
