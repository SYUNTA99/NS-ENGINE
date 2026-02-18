# 39: DeviceLost + MemoryStats

## 目的
デバイスロスト復旧とメモリ使用量統計。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md §DeviceLost
- source/engine/RHI/Public/RHIDeviceLost.h
- source/engine/RHI/Public/RHIMemoryStats.h

## TODO
- [ ] DXGI_ERROR_DEVICE_REMOVED検出 + GetDeviceRemovedReason
- [ ] DRED: ID3D12DeviceRemovedExtendedDataSettings + ページフォルト診断（±16MB範囲検索）
- [ ] DeviceLost復旧: Submission/Interruptキュー排出 → 全Payload解放 → 全リソース解放 → デバイス再作成
- [ ] ヒープ使用量追跡 + アロケータ別統計 + HUD表示用データ提供

## 制約
- DeviceLost後: Submission/Interruptキューに残留Payloadあり → 排出してから解放しないとメモリリーク
- 復旧順序: キュー排出 → BlockUntilIdle(タイムアウト付き) → 全D3D12オブジェクト解放 → デバイス再作成 → リソース再構築
- E_OUTOFMEMORY: 専用ハンドリング（GPUOutOfMemoryDelegate通知）

## 完了条件
- デバイスロスト検出→エラー報告→キュー排出、メモリ使用量統計表示

## 見積もり
- 新規ファイル: 2 (D3D12DeviceLost.h/.cpp)
