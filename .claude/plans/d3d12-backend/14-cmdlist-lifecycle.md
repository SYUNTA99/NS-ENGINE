# 14: CommandList管理

## 目的
CommandAllocatorとCommandListのライフサイクル管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md §リサイクル

## TODO
- [ ] Allocator + CommandListのペアリング管理
- [ ] フレーム単位のリサイクルシステム
- [ ] 非同期コンピュートコマンドリスト対応

## 完了条件
- CommandList生成→記録→Close→Execute→フェンス待ち→リサイクル一連動作

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12CommandAllocator/List/Context
