# 38: Residency Manager

## 目的
GPUメモリレジデンシー管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md §Residency
- source/engine/RHI/Public/RHIResidency.h

## TODO
- [ ] IDXGIAdapter3::QueryVideoMemoryInfo + メモリ予算モニタリング
- [ ] ResidencySet操作: Open → リソース使用時Insert(Handle) → Close → MakeResident/Evict判定
- [ ] MakeResident / Evict + LRUベースのページング判定
- [ ] ResidencyHandle初期化: Initialize(Handle, ID3D12Pageable*, Size) → BeginTrackingObject

## 制約
- ResidencySet: Open/Close外でのInsertは禁止（アサーション失敗）
- ResidencyHandle: IsInitialized()チェック後にのみInsert可能
- コマンドリスト記録時に使用リソースをResidencySetに登録 → Close時にMakeResident実行

## 完了条件
- メモリ予算超過時の自動Evict、ResidencySet Open/Close動作

## 見積もり
- 新規ファイル: 2 (D3D12Residency.h/.cpp)
