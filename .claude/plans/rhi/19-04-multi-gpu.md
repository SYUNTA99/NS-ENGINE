# 19-04: マルチGPU

## 状態: implemented

## 概要
マルチGPU（Linked Adapter / Explicit Multi-GPU）のノード管理、クロスノードリソース共有、AFR/SFRレンダリング戦略を定義する。

## 依存
- Part 01（RHITypes: GPUMask）
- Part 09（同期: IRHIFence）
- Part 01（リソース基底: IRHIResource）
- Part 20（IRHIDevice）

## ファイル
- `source/engine/RHI/Public/RHIMultiGPU.h`

## 既存基盤（実装済み）
| 型 | ファイル |
|---|---------|
| `GPUMask` | RHITypes.h |
| `GPUIndex` | RHITypes.h |
| `nodeMask` フィールド | IRHIPipelineState.h, RHIQuery.h等 |
| `RHIAdapterDesc::numDeviceNodes` | RHIAdapterDesc.h |
| `IRHIDevice::GetGPUMask/GetGPUIndex` | IRHIDevice.h |

## 定義型一覧

### 型エイリアス・定数
| 名前 | 値 | 説明 |
|------|---|------|
| `ERHIGPUNode` | uint32 | GPUノードインデックス型 |
| `kInvalidGPUNode` | ~0u | 無効ノード |
| `kMaxGPUNodes` | 4 | 最大サポートGPUノード数 |

### 構造体
| 型 | 説明 |
|---|------|
| `RHINodeAffinityMask` | ビットマスク + ヘルパー (All/Single/Node0/Contains/CountNodes/IsSingleNode/GetFirstNode/ToGPUMask/FromGPUMask/演算子) |
| `RHIMultiGPUCapabilities` | nodeCount, crossNodeSharing/Copy/TextureSharing/Atomics, isLinkedAdapter, IsMultiGPU()/IsSingleGPU() |
| `RHICrossNodeResourceDesc` | sourceNode, destNode, リソース参照, creation/visibleNodeMask |
| `RHICrossNodeCopyDesc` | source/destリソース + source/destノード |
| `RHICrossNodeFenceSync` | fence + signalNode/waitNode + fenceValue |

### インターフェース
| 型 | 継承 | 説明 |
|---|------|------|
| `IRHIMultiGPUDevice` | - | ノード管理、クロスノード操作 |

### IRHIMultiGPUDeviceメソッド
```cpp
virtual uint32 GetNodeCount() const = 0;
virtual IRHIDevice* GetNodeDevice(ERHIGPUNode node) const = 0;
RHINodeAffinityMask GetAllNodesMask() const; // インライン
virtual IRHIResource* CreateCrossNodeResource(const RHICrossNodeResourceDesc& desc) = 0;
virtual void CrossNodeCopy(const RHICrossNodeCopyDesc& desc) = 0;
virtual void SignalCrossNode(const RHICrossNodeFenceSync& sync) = 0;
virtual void WaitCrossNode(const RHICrossNodeFenceSync& sync) = 0;
```

### レンダリングヘルパー
| 型 | 説明 |
|---|------|
| `RHIAlternateFrameRenderer` | AFR: フレームごとにGPU切替 (Initialize/GetCurrentNode/GetCurrentNodeMask/AdvanceFrame/GetCurrentFrame/GetNodeCount) |
| `RHISplitFrameRenderer` | SFR: 画面分割 (Initialize/GetHorizontalSplitRegion/GetVerticalSplitRegion/GetNodeCount + SplitRegionネスト構造体) |

### IRHIDevice追加メソッド
```cpp
virtual RHIMultiGPUCapabilities GetMultiGPUCapabilities() const = 0;
virtual uint32 GetNodeCount() const = 0;
```

## 設計ノート
- `RHINodeAffinityMask`はD3D12のCreationNodeMask/VisibleNodeMaskに対応
- `GPUMask`（既存）との相互変換: `ToGPUMask()` / `FromGPUMask()`
- AFR: VR等で有効、フレームレイテンシが増加するがスループット向上
- SFR: 水平/垂直分割に対応、合成時にクロスノードコピーが必要
- シングルGPU環境では`IRHIMultiGPUDevice`は不要（`GetNodeCount()==1`で判定）
