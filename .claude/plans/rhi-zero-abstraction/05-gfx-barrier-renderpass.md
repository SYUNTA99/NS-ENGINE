# 05: GraphicsContext — Barrier+RenderPass+Misc inline化

## 対象ファイル
- `source/engine/RHI/Public/IRHICommandContext.h`

## 変換する関数（10個）

### バリアバッチ（4個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `TransitionBarrier(resource, before, after, sub)` | `TransitionBarrier` | subresource = kAllSubresources |
| `TransitionBarriers(barriers, count)` | `TransitionBarriers` | — |
| `UAVBarriers(barriers, count)` | `UAVBarriers` | — |
| `AliasingBarriers(barriers, count)` | `AliasingBarriers` | — |

### RenderPass lifecycle（2個）
| メソッド | DispatchTableエントリ | 参照→ポインタ |
|---------|---------------------|-------------|
| `BeginRenderPass(desc)` | `BeginRenderPass` | `const RHIRenderPassDesc&` → `&desc` |
| `EndRenderPass()` | `EndRenderPass` | — |

### その他（4個）
| メソッド | DispatchTableエントリ |
|---------|---------------------|
| `SetPredication(buffer, offset, op)` | `SetPredication` |
| `NextSubpass()` | `NextSubpass` |
| `CommitBuffer(buffer, newCommitSize)` | `CommitBuffer` |
| `CommitTextureRegions(tex, regions, count, commit)` | `CommitTextureRegions` |

## virtual維持（明示的決定）
以下はCOLD（ステート問い合わせ/診断）のためvirtual維持:
- `IsInRenderPass()` — bool問い合わせ
- `GetCurrentRenderPassDesc()` — ポインタ返却
- `GetCurrentSubpassIndex()` — uint32返却
- `GetRenderPassStatistics(outStats)` — 診断
- `ResetStatistics()` — 診断

## 注意
- `AliasingBarriers` にはvirtual版（ポインタ+count）と非virtual版（const ref batch）がある。virtual版のみinline化。
- `ClearPredication()` は非virtualラッパー。変更不要。

## TODO
- [ ] Barrier batch 4個 inline化（TransitionBarrierのデフォルト引数維持）
- [ ] BeginRenderPass, EndRenderPass inline化（参照→ポインタ変換注意）
- [ ] Predication+NextSubpass+Commit 4個 inline化
- [ ] AliasingBarriers の2つのオーバーロード整理
- [ ] ビルド確認
