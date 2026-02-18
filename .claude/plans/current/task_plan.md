# タスク計画: D3D12RHI コードレビュー修正

Mode: implementation

## 状態: complete

## ゴール

コードレビューで検出された有効な10件のバグ・品質問題を修正する。

## 修正結果

### 即座に修正（正確性バグ）— 全完了

| # | ファイル | 問題 | 修正内容 |
|---|---------|------|---------|
| 1 | D3D12Barriers.h/cpp | バリアサイレントドロップ | バッチ満杯時にauto-flush→再追加。SetCommandList()でcmdListバインド |
| 2 | D3D12CommandContext.h/cpp + D3D12Dispatch.cpp | アップロードバッファUAF | DeferRelease()で一時ComPtrをコンテキストに保持、Reset()でクリア |
| 3 | D3D12View.h/cpp + D3D12Sampler.h/cpp | ディスクリプタヒープリーク | ComPtr<ID3D12DescriptorHeap> descriptorHeap_をメンバに保持。Detach()廃止 |
| 4 | D3D12Fence.cpp | HRESULT未チェック | Signal/SetEventOnCompletionの戻り値をチェック+LOG_HRESULT |
| 5 | D3D12RootSignature.cpp | paramCount_不整合 | 実際にコピーした数をparamCount_/staticSamplerCount_に設定+LOG_WARN |
| 6 | D3D12Dispatch.cpp | reinterpret_cast安全性 | static_assert(sizeof一致)をViewport/Rect変換に追加 |

### 設計改善 — 完了

| # | ファイル | 問題 | 修正内容 |
|---|---------|------|---------|
| 7 | D3D12Dispatch.cpp | dynamic_castホットパス | GetQueueType()判別+static_castに変更（毎フレーム数百回のRTTI回避） |

### ドキュメント・防御的修正 — 完了

| # | ファイル | 問題 | 修正内容 |
|---|---------|------|---------|
| 8 | D3D12CommandAllocator.cpp | スレッド安全性 | 単一スレッド前提のコメント追加 |

## ビルド検証

- Debug: D3D12RHI.lib 0エラー ✓
- Release: D3D12RHI.lib 0エラー ✓
