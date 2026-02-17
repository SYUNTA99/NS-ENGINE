# 04: GraphicsContext — RT+Viewport+Clear+DynState inline化

## 対象ファイル
- `source/engine/RHI/Public/IRHICommandContext.h`

## 変換する関数（11個）

### レンダーターゲット+ビューポート+クリア（5個）
| メソッド | DispatchTableエントリ |
|---------|---------------------|
| `SetRenderTargets(numRTVs, rtvs, dsv)` | `SetRenderTargets` |
| `SetViewports(count, viewports)` | `SetViewports` |
| `SetScissorRects(count, rects)` | `SetScissorRects` |
| `ClearRenderTargetView(rtv, color)` | `ClearRenderTargetView` |
| `ClearDepthStencilView(dsv, clearD, d, clearS, s)` | `ClearDepthStencilView` |

### 動的ステート（6個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `SetBlendFactor(factor)` | `SetBlendFactor` | — |
| `SetStencilRef(refValue)` | `SetStencilRef` | — |
| `SetLineWidth(width)` | `SetLineWidth` | — |
| `SetDepthBounds(min, max)` | `SetDepthBounds` | — |
| `SetShadingRate(rate, combiners)` | `SetShadingRate` | combiners = nullptr |
| `SetShadingRateImage(vrsImage)` | `SetShadingRateImage` | — |

## 非virtualラッパー（変更不要）
- `SetRenderTarget(rtv, dsv)`, `SetViewport(vp)`, `SetScissorRect(rect)`
- `SetViewportAndScissor(vp)`, `ClearDepth(dsv, d)`, `ClearStencil(dsv, s)`
- `DisableDepthBounds()`

## TODO
- [ ] RT+Viewport+Clear 5個 inline化
- [ ] DynState 6個 inline化（デフォルト引数維持）
- [ ] 非virtualラッパー7個の動作確認
- [ ] ビルド確認
