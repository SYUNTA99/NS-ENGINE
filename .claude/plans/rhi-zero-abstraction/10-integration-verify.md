# 10: 統合検証 + Private/*.cpp整理

## 対象ファイル
- `source/engine/RHI/Public/RHICommands.h` — CmdXxx照合
- `source/engine/RHI/Internal/RHICommandBuffer.h` — switchケース照合
- `source/engine/RHI/Private/*.cpp` — スタブ一括更新

## 作業内容

### 1. CmdXxx / switch / DispatchTable 3点照合

```
ERHICommandType enum  ←→  CmdXxx 構造体  ←→  switch ケース
        ↕                                         ↕
  DispatchTable エントリ  ←→  NS_RHI_DISPATCH 使用箇所
```

全てが1:1対応していること。不足があれば追加。

### 2. Private/*.cpp スタブ一括更新

01-06でvirtualを排除した結果、対応する.cppスタブの純粋仮想関数の
実装プレースホルダが不要になった場合、削除または更新。

対象.cppファイル:
- IRHICommandContextBase関連
- IRHIComputeContext関連
- IRHICommandContext関連
- IRHIViews関連（07）
- IRHIBuffer関連（08）

### 3. HOTパス virtual残存チェック

```bash
# Context HOTパスにvirtualが残っていないこと
grep -n "virtual" IRHICommandContextBase.h  # → lifecycle/property のみ (≤10)
grep -n "virtual" IRHIComputeContext.h       # → WARM 2個 + デストラクタ (≤4)
grep -n "virtual" IRHICommandContext.h       # → RenderPass query等 (≤10)

# View/Buffer HOT accessorにvirtualが残っていないこと
grep "virtual.*Get\(GPU\|CPU\)Handle\|virtual.*GetBindlessIndex\|virtual.*GetGPUVirtualAddress\|virtual.*GetSize\b\|virtual.*GetStride" IRHIViews.h IRHIBuffer.h
# → 0件
```

### 4. 全構成ビルド

- Debug ビルド成功
- Release ビルド成功
- Burst ビルド成功
- Shipping ビルド成功

## TODO
- [ ] ERHICommandType vs CmdXxx vs switch 3点照合、不足追加
- [ ] Private/*.cpp スタブ更新（不要な純粋仮想プレースホルダ削除）
- [ ] HOTパス virtual残存 grep チェック
- [ ] 全4構成ビルド成功確認
- [ ] 完了報告 + README.md のサブ計画状態更新
