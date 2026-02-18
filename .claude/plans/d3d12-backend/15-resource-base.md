# 15: Resource基底

## 目的
D3D12リソースの基底クラスとIRHIResource実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md
- source/engine/RHI/Public/IRHIResource.h

## TODO
- [ ] D3D12Resource.h/.cpp — ID3D12Resourceラッパー基底 + IRHIResource実装
- [ ] リソース状態追跡（初期状態管理）
- [ ] GPUVirtualAddressキャッシュ + Map/Unmapサポート
- [ ] DeferredDeleteキュー: フェンス値ベースの安全なリソース解放（全キュー完了後に実削除）
- [ ] bDeferDelete=trueデフォルト、Resource/Heap/Descriptor/CommandAllocator等の遅延削除パイプライン

## 制約
- D3D12ではGPU使用中のリソース破棄は未定義動作 → 全オブジェクトに遅延削除必須
- DeferredDeleteキューの配置: Device/Adapterレベルに実体を持つ。Resource基底はエンキューのみ担当、排出は06(DynamicRHI Shutdown)で実行
- 削除タイミング: 全キューのフェンス完了値を確認後にバッチ削除
- 対象: ID3D12Resource, ID3D12Heap, DescriptorHandle, CommandAllocator等

## 完了条件
- Resource基底が他リソースクラスの基底として使用可能、DeferredDelete動作確認

## 見積もり
- 新規ファイル: 2 (D3D12Resource.h/.cpp)
