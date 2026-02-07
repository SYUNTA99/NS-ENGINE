# RHI コードレビュー P0 修正計画

## Mode: implementation

## Goal
コードレビューで発見された CRITICAL 12件のうち、P0（即時修正必須）4件を修正する。

## P0 修正対象

| # | ファイル | 問題 | 状態 |
|---|---------|------|------|
| 1 | IRHIResource.h | `m_pendingDelete` データ競合 → `std::atomic<bool>` | **done** |
| 2 | RHIMemoryTypes.h / RHIBarrier.h | `RHIAliasingBarrier` 重複定義 → RHIMemoryTypes.hから削除 | **done** |
| 3 | IRHISampler.h | `operator==` がハッシュ比較のみ → フィールド完全比較 | **done** |
| 4 | RHIMacros.h | `RHI_ENUM_CLASS_FLAGS` が `uint8` enum で破壊 → `underlying_type_t` | **done** |

## P1 修正対象（次回）

| # | ファイル | 問題 |
|---|---------|------|
| 5 | RHIEnums.h:435 | マジックナンバー 14 |
| 6 | IRHITexture.h:1088 | do-while 空レンジ問題 |
| 7 | RHIBufferAllocator.h:48 | 整数オーバーフロー |
| 8 | IDynamicRHI.h:314 | g_dynamicRHI 非atomic |
