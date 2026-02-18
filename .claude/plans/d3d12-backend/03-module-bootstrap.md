# 03: モジュールブートストラップ

## 目的
D3D12RHIModuleのIDynamicRHIModule実装とエンジン起動時の選択パス。

## 参照
- source/engine/RHI/Public/IDynamicRHI.h

## TODO
- [ ] D3D12RHI/Private/D3D12RHIModule.cpp — IDynamicRHIModule実装
- [ ] D3D12RHIModule::CreateRHI() → D3D12DynamicRHIインスタンス返却（スタブ）
- [ ] エンジン起動時のD3D12バックエンド選択パス
- [ ] 4構成ビルド通過確認（D3D12RHI.lib生成）

## 完了条件
- 4構成ビルド通過、D3D12RHI.lib生成、リンクエラーなし

## 見積もり
- 新規ファイル: 1
