# RHI Zero-Abstraction: 条件コンパイル + コマンドリスト化

## 目的

RHIのホットパスからvirtual関数呼び出しを排除し、**フレームあたりのvirtual呼び出しを数回に削減**する。

- コマンド記録: POD構造体 → リニアアロケータ（virtual 0回）
- コマンド実行: DispatchTable（開発）/ 条件コンパイル直接呼出（出荷）
- バックエンド入口/出口: Begin/Finish/Submit のみvirtual（フレームあたり数回）

## 影響範囲

- `source/engine/RHI/Public/` — Context系ヘッダー 4ファイル + View/Buffer 3ファイル
- `source/engine/RHI/Public/RHIDispatchTable.h` — エントリ照合
- `source/engine/RHI/Internal/RHICommandBuffer.h` — カバレッジ照合
- `source/engine/RHI/Private/` — 対応.cppスタブ群
- `source/common/utility/macros.h` — NS_BUILD_SHIPPING
- `premake5.lua` — ビルド構成

## サブ計画

### Phase 1: ContextBase + Compute（2サブ計画）

| # | 計画 | 状態 | 対象 |
|---|------|------|------|
| 01 | [ContextBase: 全HOT inline化](01-ctxbase-all.md) | complete | 14 virtual + 3 DT-only → ビルドOK |
| 02 | [ComputeContext: 全HOT inline化](02-compute-all.md) | complete | 17 virtual → ビルドOK |

### Phase 2: GraphicsContext（4サブ計画）

| # | 計画 | 状態 | 対象 |
|---|------|------|------|
| 03 | [GraphicsContext: Draw+Pipeline+Bind](03-gfx-draw-pipeline-bind.md) | complete | 16 virtual → ビルドOK |
| 04 | [GraphicsContext: RT+Viewport+DynState](04-gfx-rt-viewport-dynstate.md) | complete | 11 virtual → ビルドOK |
| 05 | [GraphicsContext: Barrier+RenderPass+Misc](05-gfx-barrier-renderpass.md) | complete | 10 virtual → ビルドOK |
| 06 | [GraphicsContext: Mesh+RT+WorkGraph+Misc](06-gfx-mesh-rt-wg.md) | complete | 14 virtual → ビルドOK |

### Phase 3: View/Buffer キャッシュ化（2サブ計画）

| # | 計画 | 状態 | 対象 |
|---|------|------|------|
| 07 | [View handle cache (5クラス)](07-view-cache.md) | complete | 12 cache (5クラス) → ビルドOK |
| 08 | [Buffer accessor cache](08-buffer-cache.md) | complete | 3 cache → ビルドOK |

### Phase 4: Infrastructure整備（2サブ計画）

| # | 計画 | 状態 | 対象 |
|---|------|------|------|
| 09 | [DispatchTable照合 + Build Shipping](09-dispatch-build.md) | complete | DT照合OK + Shipping構成追加 → ビルドOK |
| 10 | [統合検証 + Private/*.cpp整理](10-integration-verify.md) | complete | 3-point照合OK, stubs OK, 4構成ビルドOK |

## 依存関係

```
01 → 02 → 03 → 04 → 05 → 06
                              ↓
         07 → 08 ──────────→ 09 → 10
```

- Phase 1-2 は順序依存（継承階層: Base→Compute→Graphics）
- Phase 3 はPhase 1完了後なら独立実行可
- Phase 4 は Phase 1-3 全完了後

## 検証方法（最終）

- 全構成ビルド成功（Debug/Release/Burst/Shipping）
- Context HOTパスに `virtual` が残っていないこと（grep）
- View/Buffer HOT accessorに `virtual` が残っていないこと（grep）
- DispatchTable.IsValid() が全エントリカバー
- ERHICommandType::Count = CmdXxx数 = switchケース数

## 共通注意事項

### デフォルト引数の保持
inline化時、元のvirtualメソッドにデフォルト引数がある場合はそのまま維持すること。
DispatchTable関数ポインタにはデフォルト引数がないが、inlineメソッド側で保持する。
```cpp
// 例: UAVBarrier(resource = nullptr)
void UAVBarrier(IRHIResource* resource = nullptr)
{
    NS_RHI_DISPATCH(UAVBarrier, this, resource);
}
```

### 参照→ポインタ変換
Contextメソッドが `const T&`（参照）、DispatchTableが `const T*`（ポインタ）の場合、
`&desc` で変換すること。対象箇所:

| メソッド | Context | DispatchTable |
|---------|---------|---------------|
| `DispatchGraph` | `const RHIWorkGraphDispatchDesc&` | `*` |
| `InitializeWorkGraphBackingMemory` | `const RHIWorkGraphBackingMemory&` | `*` |
| `BeginRenderPass` | `const RHIRenderPassDesc&` | `*` |
| `BuildRaytracingAccelerationStructure` | `const RHIAccelerationStructureBuildDesc&` | `*` |
| `DispatchRays` | `const RHIDispatchRaysDesc&` | `*` |
| `SetIndexBuffer` | `const RHIIndexBufferView&` | `*` |

## virtual維持（明示的決定）

以下は意図的にvirtual維持する関数。変換対象外。

### IRHICommandContextBase — ライフサイクル+プロパティ（8個）
GetDevice, GetGPUMask, GetQueueType, GetPipeline, Begin, Finish, Reset, IsRecording

### IRHIImmediateContext（2個）
Flush, GetNativeContext — COLD（デバッグ/初期化専用）

### IRHIComputeContext — WARM getter（2個）
GetCBVSRVUAVHeap, GetSamplerHeap — フレームあたり数回のステート問い合わせ

### IRHICommandContext — RenderPass state query（5個）
IsInRenderPass, GetCurrentRenderPassDesc, GetCurrentSubpassIndex, GetRenderPassStatistics, ResetStatistics — COLD（ステート問い合わせ/診断）

### DispatchTable-only（virtual宣言なし、inline新規追加も不要）
GetQueryResult — wait可能性ありCmdXxx/switchからのみ使用

## 設計決定

| 決定 | 理由 |
|------|------|
| virtualゼロではなく「フレームあたり数回」 | Begin/Finish/Submitはvirtual。RenderPass開閉はDTエントリがありinline化。完全排除は過剰 |
| IRHIUploadContextパターンを踏襲 | 既に正しく動作している模範実装 |
| IDynamicRHI/IRHIDevice/IRHIQueueはvirtual維持 | COLD path。フレームあたり数回 |
| View handle値をPODキャッシュ | 作成時に値確定。毎回virtual不要 |
| GraphicsContextを機能領域で4分割 | 52 virtualを一括変更は危険 |
| Phase 4でPrivate/*.cppを一括整理 | 個別にやると手戻りが多い |
