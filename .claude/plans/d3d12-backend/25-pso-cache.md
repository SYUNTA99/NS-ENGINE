# 25: PSOキャッシュ + ステート変換

## 目的
PSOキャッシュとRHI→D3D12ステート変換ヘルパー。

## 参照
- docs/D3D12RHI/D3D12RHI_Part08_PipelineState.md §Cache

## TODO
- [ ] ハッシュベースPSOキャッシュ（CombinedHash → PSO*、RWLock保護: Read=ヒット検索、Write=ミス挿入+DoubleCheck）
- [ ] PSOディスクキャッシュ（シリアライズ/デシリアライズ、ドライババージョン変更時は自動クリア+再構築）
- [ ] ERHICullMode → D3D12_CULL_MODE等ステート変換ヘルパー
- [ ] ERHIBlendFactor/CompareFunc/Topology等の変換

## 制約
- PSOキャッシュはマルチスレッドアクセス必須: FRWLock（Read多/Write少パターン）
- ディスクキャッシュ: ドライババージョン不一致時はキャッシュ無効化→再ビルド（stale PSO防止）

## 完了条件
- PSO重複作成なし、スレッドセーフ確認、ステート変換の網羅性確認

## 見積もり
- 新規ファイル: 2 (D3D12StateConvert.h/.cpp)
