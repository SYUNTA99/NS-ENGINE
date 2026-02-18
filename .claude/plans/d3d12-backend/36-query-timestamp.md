# 36: QueryHeap + Timestamp

## 目的
GPUクエリヒープとタイムスタンプ計測。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md §Profiling
- source/engine/RHI/Public/RHIQuery.h

## TODO
- [ ] クエリヒープ作成: TIMESTAMP(Direct用) + COPY_QUEUE_TIMESTAMP(Copy用) + OCCLUSION + PIPELINE_STATISTICS
- [ ] ResolveQueryData → Readbackバッファ（永続Map、Unmapしない）
- [ ] BeginTimestamp / EndTimestamp
- [ ] GPU周波数取得: GetTimestampFrequency（キュー毎に呼出し、Direct/Copyで値が異なる場合あり）

## 制約
- DirectキューとCopyキューでタイムスタンプヒープタイプが異なる（TIMESTAMP vs COPY_QUEUE_TIMESTAMP）
- Readbackバッファは永続Map（D3D12ではREADBACKヒープのUnmap後再Mapが非効率）
- クエリ結果はSyncPoint待機後にのみCPU読取可能

## 完了条件
- GPUタイムスタンプ取得・表示（Direct/Copyキュー両対応）

## 見積もり
- 新規ファイル: 2 (D3D12Query.h/.cpp)
