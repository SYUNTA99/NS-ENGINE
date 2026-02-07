# 02-06: ワークグラチ

## 目的

GPU上で動的にワークを生成・実行するワークグラフシステムのインターフェースを定義する。

## 参照ドキュメント

- .claude/plans/rhi/docs/RHI_Implementation_Guide_Part4.md (2. ワークグラチ

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIWorkGraph.h`
- `Source/Engine/RHI/Public/RHIWorkGraphTypes.h`

## ワークグラフの概念

```
従来のモデル:
  CPU ────▶ Dispatch(X, Y, Z) ────▶ GPU実行
       │
       │ ワーク量のCPUが事前に決定
       ▼

ワークグラフモデル:
  CPU ────▶ DispatchGraph(初期ワーク) ────▶ GPU
                                              │
                                       ┌──────┴──────│
                                       │Node A      │
                                       │(シェーダー) │
                                       └──────┬──────│
                                        出力レコード
                                    ┌─────┴─────│
                              ┌─────┴─────│┌───┴───│
                              │Node B    ││Node C │
                              └───────────│└───────│
                              GPUがワークを動的に生成。
```

## TODO

### ワークグラフノードタイプ

```cpp
/// ワークグラフノードタイプ
enum class ERHIWorkGraphNodeType : uint8 {
    /// ブロードキャストノード(スレッドグループ単位
    Broadcasting,

    /// コアレッシングノード(スレッド単位、グループ化)
    Coalescing,

    /// スレッドノード(単一スレッド)
    Thread
};

/// ノード起動モード
enum class ERHIWorkGraphLaunchMode : uint8 {
    /// 通常起動(スレッドグループ単位
    Normal,

    /// スレッド単位起動
    PerThread
};
```

### ワークグラフノード記述

```cpp
/// ワークグラフノードの入力レコード
struct RHIWorkGraphInputRecord {
    const void* data;           // 入力データポインタ
    uint32 sizeInBytes;         // レコードサイズ
    uint32 count;               // レコード数
};

/// ワークグラフノード定義
struct RHIWorkGraphNodeDesc {
    const char* name;                       // ノード名
    const char* shaderEntryPoint;           // シェーダーエントリーポイント
    ERHIWorkGraphNodeType nodeType;         // ノードタイプ
    uint32 maxRecursionDepth;               // 最大再帰深度
    bool isEntryPoint;                      // エントリーポイントか
    uint32 maxDispatchGrid[3];              // 最大ディスパッチグリッド
};

/// ノード間接続
struct RHIWorkGraphEdge {
    uint32 sourceNodeIndex;     // 出力元ノード
    uint32 destNodeIndex;       // 入力先ノード
    uint32 outputSlot;          // 出力スロット
};
```

### ワークグラフパイプラインステート

```cpp
/// ワークグラフパイプライン記述
struct RHIWorkGraphPipelineDesc {
    /// シェーダーライブラリ (DXILライブラリ)
    IRHIShaderLibrary* shaderLibrary;

    /// ノード定義配列
    const RHIWorkGraphNodeDesc* nodes;
    uint32 nodeCount;

    /// エッジ定義配列
    const RHIWorkGraphEdge* edges;
    uint32 edgeCount;

    /// グローバルルートシグネチャ
    IRHIRootSignature* globalRootSignature;

    /// プログラム合
    const char* programName;

    /// デバッグ名
    const char* debugName;
};

/// ワークグラフパイプラインステート
class IRHIWorkGraphPipeline : public IRHIResource {
public:
    /// プログラム識別子取得
    virtual uint64 GetProgramIdentifier() const = 0;

    /// バッキングメモリ要件取得
    virtual uint64 GetBackingMemorySize() const = 0;

    /// ノード数取得
    virtual uint32 GetNodeCount() const = 0;

    /// ノードインデックス取得
    virtual int32 GetNodeIndex(const char* nodeName) const = 0;

    /// エントリーポイント数取得
    virtual uint32 GetEntryPointCount() const = 0;
};
```

### バッキングメモリ

```cpp
/// ワークグラフ実行用バッキングメモリ
struct RHIWorkGraphBackingMemory {
    IRHIBuffer* buffer;         // バッキングバッファ
    uint64 offset;              // オフセット
    uint64 size;                // サイズ
};

/// バッキングメモリ要件
struct RHIWorkGraphMemoryRequirements {
    uint64 minSize;             // 最小サイズ
    uint64 maxSize;             // 最大サイズ (無制限の場合はUINT64_MAX)
    uint64 sizeGranularity;     // サイズ粒度
};
```

### ワークグラフディスパッチ

```cpp
/// ディスパッチモード
enum class ERHIWorkGraphDispatchMode : uint8 {
    /// CPUから初期入力を提供
    Initialize,

    /// 前回の実行からの残りワークを継続
    Continue
};

/// ワークグラフディスパッチ記述
struct RHIWorkGraphDispatchDesc {
    /// パイプライン
    IRHIWorkGraphPipeline* pipeline;

    /// バッキングメモリ
    RHIWorkGraphBackingMemory backingMemory;

    /// ディスパッチモード
    ERHIWorkGraphDispatchMode mode;

    /// エントリーポイントノード名（Initialize時）
    const char* entryPointName;

    /// 入力レコード（Initialize時）
    RHIWorkGraphInputRecord inputRecords;
};
```

### コマンドコンテキスト拡張

```cpp
class IRHICommandContext {
    // ... 既存メソッド...

    /// ワークグラフパイプライン設定
    virtual void SetWorkGraphPipeline(IRHIWorkGraphPipeline* pipeline) = 0;

    /// ワークグラフディスパッチ
    virtual void DispatchGraph(const RHIWorkGraphDispatchDesc& desc) = 0;

    /// バッキングメモリ初期化
    virtual void InitializeWorkGraphBackingMemory(
        IRHIWorkGraphPipeline* pipeline,
        const RHIWorkGraphBackingMemory& memory
    ) = 0;
};
```

### デバイス拡張

```cpp
class IRHIDevice {
    // ... 既存メソッド...

    /// ワークグラフパイプライン作成
    virtual IRHIWorkGraphPipeline* CreateWorkGraphPipeline(
        const RHIWorkGraphPipelineDesc& desc
    ) = 0;

    /// ワークグラフサポート確認
    virtual bool SupportsWorkGraphs() const = 0;

    /// バッキングメモリ要件取得
    virtual RHIWorkGraphMemoryRequirements GetWorkGraphMemoryRequirements(
        IRHIWorkGraphPipeline* pipeline
    ) const = 0;
};
```

### ユースケース

```
1. LODカリング:
   ┌────────────│
   │Culling    │↔ 全オブジェクトを入力
   │Node       │
   └──────┬─────│
          │可視オブジェクトのみ出力
          ▼
   ┌────────────│
   │Draw Node  │↔ 描画実行
   └────────────│

2. プロシージャル生成:
   ┌────────────│
   │Generator  │↔ シードの力
   │Node       │
   └──────┬─────│
          │生成されたプリミティブを出力
          ▼
   ┌────────────│
   │Process    │↔ 後処理
   │Node       │
   └────────────│

3. パーティクルシミュレーション:
   ┌────────────│
   │Simulate   │↔ パーティクル更新
   │Node       │
   └──────┬─────│
          │生存パーティクル + 新規生成
          ▼
   ┌────────────│
   │Emit Node  │↔ 新規パーティクル生。
   └────────────│
```

### 使用パターン

```cpp
// 1. ワークグラフパイプライン作成
RHIWorkGraphPipelineDesc pipelineDesc;
pipelineDesc.shaderLibrary = shaderLib;
pipelineDesc.nodes = nodeDescs;
pipelineDesc.nodeCount = 2;
pipelineDesc.programName = "LODCulling";

auto pipeline = device->CreateWorkGraphPipeline(pipelineDesc);

// 2. バッキングメモリ確保
auto memReqs = device->GetWorkGraphMemoryRequirements(pipeline);
auto backingBuffer = device->CreateBuffer({memReqs.minSize, ...});

RHIWorkGraphBackingMemory backing;
backing.buffer = backingBuffer;
backing.offset = 0;
backing.size = memReqs.minSize;

// 3. バッキングメモリ初期化
context->InitializeWorkGraphBackingMemory(pipeline, backing);

// 4. ディスパッチ
RHIWorkGraphDispatchDesc dispatchDesc;
dispatchDesc.pipeline = pipeline;
dispatchDesc.backingMemory = backing;
dispatchDesc.mode = ERHIWorkGraphDispatchMode::Initialize;
dispatchDesc.entryPointName = "CullingEntry";
dispatchDesc.inputRecords = {objectData, sizeof(ObjectDesc), objectCount};

context->SetWorkGraphPipeline(pipeline);
context->DispatchGraph(dispatchDesc);
```

## 検証方法

- ノード定義の正確性
- バッキングメモリ要件の取得
- ディスパッチ実行の動作
- ノード間データフローの検証
- 再帰ワーク生成の検証

## 備考

ワークグラフのD3D12 (Shader Model 6.8+) でサポート、
Vulkanでは限定的にVK_AMDX_shader_enqueueで類似機能が提供される予定、
