# 17-04: GPU Breadcrumbsシステム

## 目的

GPUコマンドの実行履歴を追跡し、クラッシュ時の診断を支援するBreadcrumbシステムを定義する。

## 参照ドキュメント

- .claude/plans/rhi/docs/RHI_Implementation_Guide_Part7.md (1. GPU Breadcrumbs)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIBreadcrumbs.h`
- `Source/Engine/RHI/Private/RHIBreadcrumbs.cpp`

## Breadcrumbの概念

```
GPUクラッシュ発生時の問題
┌──────────────────────────────────────────────────────────────│
│ GPUがハング/クラッシュした時、                               │
│ 「どのコマンドで問題が起きたか」がわからない                 │
└──────────────────────────────────────────────────────────────│
           │
           ▼
Breadcrumbによる解決:
┌──────────────────────────────────────────────────────────────│
│ 各GPUコマンドの前後にマーカーを挿入                         │
│ クラッシュ時に「最後に完了したマーカー」を特定             │
│                                                             │
│ Timeline:                                                   │
│ [Marker A] ↔ [Command A] ↔ [Marker B] ↔ [Command B] ↔ ...  │
│     ✓           ✓            ✓           ✓↔ クラッシュ  │
│                                                             │
│ ↔ "Command B"で問題発生と特定可能                          │
└──────────────────────────────────────────────────────────────│
```

## TODO

### ビルド設定

```cpp
/// Breadcrumbビルド設定
/// ターゲットビルドに応じて適切なマクロを定義

#if NS_DEBUG || NS_DEVELOPMENT
    /// フル機能 (Debug/Development)
    #define NS_RHI_BREADCRUMBS_FULL 1
    #define NS_RHI_BREADCRUMBS_MINIMAL 0
#elif NS_PROFILE_GPU
    /// 最小限 (ProfileGPU)
    #define NS_RHI_BREADCRUMBS_FULL 0
    #define NS_RHI_BREADCRUMBS_MINIMAL 1
#else
    /// 無効 (Release)
    #define NS_RHI_BREADCRUMBS_FULL 0
    #define NS_RHI_BREADCRUMBS_MINIMAL 0
#endif

/// CPUトレース出力
#define NS_RHI_BREADCRUMBS_EMIT_CPU (NS_RHI_BREADCRUMBS_FULL)

/// ソース位置情報
#define NS_RHI_BREADCRUMBS_EMIT_LOCATION (NS_RHI_BREADCRUMBS_FULL)
```

### Breadcrumbデータ

```cpp
/// Breadcrumbのメタデータ
struct RHIBreadcrumbData {
    /// 静的名前 (コンパイル時定数)
    const char* staticName = nullptr;

    /// ソースファイル
    const char* sourceFile = nullptr;

    /// 行番号
    uint32 sourceLine = 0;

    /// GPU統計D (プロファイリング連携用)
    uint32 statsId = 0;
};

/// Breadcrumbノード
struct RHIBreadcrumbNode {
    /// 一意のID
    uint32 id;

    /// 親ノード(nullptr = ルート）
    RHIBreadcrumbNode* parent;

    /// メタデータ
    const RHIBreadcrumbData* data;

    /// フルパス取得(デバッグ用)
    FString GetFullPath() const;

    /// クラッシュデータ書き込み
    void WriteCrashData(FStringBuilder& out) const;
};
```

### Breadcrumbアロケーター

```cpp
/// Breadcrumb専用アロケーター
class FRHIBreadcrumbAllocator {
public:
    FRHIBreadcrumbAllocator();
    ~FRHIBreadcrumbAllocator();

    /// ノードアロケーチ
    RHIBreadcrumbNode* AllocateNode(
        RHIBreadcrumbNode* parent,
        const RHIBreadcrumbData& data
    );

    /// フレーム終了時リセット
    void Reset();

private:
    /// リニアアロケーター
    FLinearAllocator m_allocator;

    /// 次のノードID
    uint32 m_nextId;
};
```

### Breadcrumb状態管理

```cpp
/// Breadcrumb状態(スレッドローカル)
class FRHIBreadcrumbState {
public:
    /// 現在のアクティブノード取得
    RHIBreadcrumbNode* GetCurrentNode() const;

    /// ノードをプッシュ
    void PushNode(RHIBreadcrumbNode* node);

    /// ノードをポップ
    void PopNode();

    /// アクティブなBreadcrumbをダンプ（クラッシュ時）
    static void DumpActiveBreadcrumbs();

private:
    /// ノードスタック
    TArray<RHIBreadcrumbNode*> m_nodeStack;

    /// スレッドローカルインスタンス
    static thread_local FRHIBreadcrumbState* s_instance;
};
```

### コマンドコンテキスト拡張

```cpp
class IRHICommandContext {
    // ... 既存メソッド...

    //=========================================================================
    // Breadcrumb
    //=========================================================================

    /// Breadcrumb開始
    virtual void BeginBreadcrumbGPU(RHIBreadcrumbNode* node) = 0;

    /// Breadcrumb終了
    virtual void EndBreadcrumbGPU(RHIBreadcrumbNode* node) = 0;
};
```

### Breadcrumbマクロ

```cpp
#if NS_RHI_BREADCRUMBS_FULL || NS_RHI_BREADCRUMBS_MINIMAL

/// 基本Breadcrumbイベント
#define RHI_BREADCRUMB_EVENT(Context, Name) \
    FRHIBreadcrumbScope NS_MACRO_CONCATENATE(_breadcrumb_, __LINE__)(Context, Name)

/// 条件付きBreadcrumbイベント
#define RHI_BREADCRUMB_EVENT_CONDITIONAL(Context, Condition, Name) \
    FRHIBreadcrumbScopeConditional NS_MACRO_CONCATENATE(_breadcrumb_, __LINE__)(Context, Condition, Name)

/// GPU統計付きBreadcrumbイベント
#define RHI_BREADCRUMB_EVENT_STAT(Context, StatName, Name) \
    FRHIBreadcrumbScopeStat NS_MACRO_CONCATENATE(_breadcrumb_, __LINE__)(Context, StatName, Name)

/// フォーマット付きBreadcrumbイベント
#define RHI_BREADCRUMB_EVENT_F(Context, Name, Format, ...) \
    FRHIBreadcrumbScopeFormatted NS_MACRO_CONCATENATE(_breadcrumb_, __LINE__)( \
        Context, Name, Format, ##__VA_ARGS__)

#else

#define RHI_BREADCRUMB_EVENT(Context, Name)
#define RHI_BREADCRUMB_EVENT_CONDITIONAL(Context, Condition, Name)
#define RHI_BREADCRUMB_EVENT_STAT(Context, StatName, Name)
#define RHI_BREADCRUMB_EVENT_F(Context, Name, Format, ...)

#endif
```

### Breadcrumbスコープ

```cpp
/// RAIIスコープ
class FRHIBreadcrumbScope {
public:
    NS_DISALLOW_COPY(FRHIBreadcrumbScope);

    FRHIBreadcrumbScope(IRHICommandContext* context, const char* name)
        : m_context(context)
        , m_node(nullptr)
    {
#if NS_RHI_BREADCRUMBS_FULL || NS_RHI_BREADCRUMBS_MINIMAL
        auto& state = FRHIBreadcrumbState::Get();
        auto& allocator = context->GetBreadcrumbAllocator();

        RHIBreadcrumbData data;
        data.staticName = name;
#if NS_RHI_BREADCRUMBS_EMIT_LOCATION
        data.sourceFile = __FILE__;
        data.sourceLine = __LINE__;
#endif

        m_node = allocator.AllocateNode(state.GetCurrentNode(), data);
        state.PushNode(m_node);
        m_context->BeginBreadcrumbGPU(m_node);
#endif
    }

    ~FRHIBreadcrumbScope() {
#if NS_RHI_BREADCRUMBS_FULL || NS_RHI_BREADCRUMBS_MINIMAL
        if (m_node) {
            m_context->EndBreadcrumbGPU(m_node);
            FRHIBreadcrumbState::Get().PopNode();
        }
#endif
    }

private:
    IRHICommandContext* m_context;
    RHIBreadcrumbNode* m_node;
};
```

### クラッシュ診断

```cpp
/// GPUクラッシュ時の診断フロー
void OnGPUCrash()
{
    // 1. Device Removed検出
    if (!device->IsDeviceValid()) {
        NS_LOG_ERROR("GPU Device Removed detected!");

        // 2. アクティブなBreadcrumbをダンプ
        FRHIBreadcrumbState::DumpActiveBreadcrumbs();
        /*
         * 出力例
         * [RHI] GPU Crash Breadcrumbs:
         * [RHI]   Scene/
         * [RHI]     BasePass/
         * [RHI]       DrawMesh [Mesh=SM_Rock LOD=0]
         * [RHI]       ↔ Last executed
         */

        // 3. ベンダー固有診断 (Aftermath等
        if (NVAftermath::IsEnabled()) {
            NVAftermath::CollectCrashDump();
        }

        // 4. クラッシュレポート生成
        GenerateCrashReport();
    }
}
```

### GPU書き込みマーカー

```cpp
/// GPU側でのマーカー書き込み (D3D12実装）。
class FD3D12BreadcrumbWriter {
public:
    FD3D12BreadcrumbWriter(ID3D12Device* device);

    /// マーカーバッファ作成
    void Initialize(uint32 maxMarkers);

    /// マーカー書き込みコマンド発行
    void WriteMarker(
        ID3D12GraphicsCommandList* cmdList,
        uint32 markerId,
        uint32 value
    );

    /// 完了したマーカーを読み取り
    uint32 GetLastCompletedMarker() const;

private:
    /// リードバックバッファ
    ComPtr<ID3D12Resource> m_markerBuffer;

    /// マップされたポインタ
    uint32* m_mappedData;
};
```

### 使用パターン

```cpp
void RenderScene(IRHICommandContext* context)
{
    RHI_BREADCRUMB_EVENT(context, "Scene");
    {
        RHI_BREADCRUMB_EVENT(context, "ShadowPass");
        RenderShadows(context);
    }

    {
        RHI_BREADCRUMB_EVENT(context, "GBuffer");
        {
            RHI_BREADCRUMB_EVENT_F(context, "DrawMesh",
                "Mesh=%s LOD=%d", meshName, lodIndex);
            DrawMesh(context, mesh);
        }
    }

    {
        RHI_BREADCRUMB_EVENT(context, "Lighting");
        ComputeLighting(context);
    }

    {
        RHI_BREADCRUMB_EVENT(context, "PostProcess");
        ApplyPostProcess(context);
    }
}
```

### NVIDIA Aftermath連携

```cpp
/// Aftermath連携マーカー
#if NS_WITH_NVIDIA_AFTERMATH
class FAftermathMarker {
public:
    FAftermathMarker(RHIBreadcrumbNode* node)
        : m_data(nullptr)
        , m_size(0)
    {
        if (Aftermath::AreMarkersEnabled()) {
            // Breadcrumbノードからマーカーデータ生成。
            m_data = GenerateMarkerData(node, m_size);
        }
    }

    void* GetData() const { return m_data; }
    uint32 GetSize() const { return m_size; }

private:
    void* m_data;
    uint32 m_size;
};
#endif
```

## 検証方法

- マーカー挿入のオーバのヘッド測定
- クラッシュ時のダンプ正確性
- 階層構造の正確性
- スレッドセーフ性
- Aftermath連携動作

## 備考

Breadcrumbシステムは開発/デバッグビルドで有効にする。
リリースビルドでは無効化してオーバのヘッドを排除、

NVIDIA Aftermath、Intel GPU Crash Dumpsと連携することで、
より詳細なクラッシュ情報を取得可能、
