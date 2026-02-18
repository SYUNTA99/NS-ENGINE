# D3D12RHI Part 6: バリアシステム (Enhanced / Legacy)

## 6.1 概要

D3D12RHIのバリアシステムは、GPUリソースの状態遷移・同期・レイアウト変更を管理する。
D3D12には2つのバリアモデルが存在し、UE5.7は両方をStrategy パターンで抽象化している。

```
+--------------------------------------------------+
|              ID3D12BarriersForAdapter             |  ← Adapter レベル抽象インターフェース
|  ConfigureDevice / CreateTransition / Release     |
|  CreateCommitted/Placed/ReservedResource          |
+-----+--------------------------------------------+
      |
      +---> FD3D12EnhancedBarriersForAdapter (D3D12_BARRIER_LAYOUT ベース)
      |
      +---> FD3D12LegacyBarriersForAdapter   (D3D12_RESOURCE_STATES ベース)


+--------------------------------------------------+
|             ID3D12BarriersForContext              |  ← コンテキストレベル抽象インターフェース
|  BeginTransitions / EndTransitions               |
|  AddGlobalBarrier / AddBarrier                   |
|  FlushIntoCommandList                            |
+-----+--------------------------------------------+
      |
      +---> FD3D12EnhancedBarriersForContext (Batcher: Global/Texture/Buffer分離)
      |
      +---> FD3D12LegacyBarriersForContext   (Batcher: D3D12_RESOURCE_BARRIER統一)
```

**コンパイル時フィーチャーフラグ:**

| フラグ | 条件 |
|--------|------|
| `D3D12RHI_SUPPORTS_ENHANCED_BARRIERS` | `D3D12_MAX_DEVICE_INTERFACE >= 11` かつ `D3D12RHI_ALLOW_ENHANCED_BARRIERS` |
| `D3D12RHI_SUPPORTS_LEGACY_BARRIERS` | `D3D12RHI_ALLOW_LEGACY_BARRIERS` |

Windows プラットフォームでは両方が有効化され、ランタイムで選択可能。


## 6.2 ED3D12Access — UE5 独自アクセスフラグ

`D3D12Access.h` で定義される `ED3D12Access` は、RHI レベルの `ERHIAccess` を D3D12 固有のアクセスフラグに拡張する。

```
enum class ED3D12Access : D3D12AccessUnderlyingType
{
    // ERHIAccess と同一の値を持つビット群
    Unknown, CPURead, Present, IndirectArgs, VertexOrIndexBuffer,
    SRVCompute, SRVGraphicsPixel, SRVGraphicsNonPixel,
    CopySrc, ResolveSrc, DSVRead,
    UAVCompute, UAVGraphics, RTV, CopyDest, ResolveDst, DSVWrite,
    BVHRead, BVHWrite, Discard, ShadingRateSource,

    // D3D12 固有拡張
    Common      = RHIAccessLast << 1,   // D3D12_RESOURCE_STATE_COMMON 相当
    GenericRead = RHIAccessLast << 2,   // D3D12_RESOURCE_STATE_GENERIC_READ 相当

    // 複合マスク
    SRVGraphics, SRVMask, UAVMask,
    ReadOnlyExclusiveMask, ReadOnlyMask, ReadableMask,
    WriteOnlyExclusiveMask, WriteOnlyMask, WritableMask,
};
```

**変換関数:**
- `ConvertToD3D12Access(ERHIAccess)` — `ERHIAccess` → `ED3D12Access` への直接キャスト
- `IsValidAccess(ED3D12Access)` — 読み書き排他性の検証 (Read + Write の不正組合せ検出)


## 6.3 Factory パターンによるプラットフォーム選択

### 6.3.1 TD3D12BarriersFactory テンプレート

`D3D12BarriersFactory.h` で定義されるテンプレートファクトリが、コンパイル時にバリア実装を選択する。

```
TD3D12BarriersFactory<
    TD3D12BarriersFactoryEntry<ImplType, AdapterImpl, ContextImpl>,
    ...
    FNullD3D12BarriersFactoryEntry  // 必須ターミネータ
>
```

**実装タイプ列挙:**
```
enum class ED3D12BarrierImplementationType
{
    Legacy,     // D3D12_RESOURCE_STATES ベース
    Enhanced,   // D3D12_BARRIER_LAYOUT ベース (D3D12 Device Interface 11+)
    Invalid,    // ターミネータ用
};
```

**コンパイル時型最適化:**

| 条件 | BarriersForAdapterType | BarriersForContextType |
|------|------------------------|------------------------|
| エントリ1個 (+ ターミネータ) | 具象型 (例: `FD3D12EnhancedBarriersForAdapter`) | 具象型 |
| エントリ2個以上 | `ID3D12BarriersForAdapter` (抽象) | `ID3D12BarriersForContext` (抽象) |

単一実装の場合は MSVC の仮想関数デバーチャライズ不足を回避するため、具象型を直接使用する。

**コンパイル時バリデーション (static_assert):**
1. エントリリストが `FNullD3D12BarriersFactoryEntry` で終端すること
2. 最低2エントリ (1実装 + ターミネータ)
3. 同一 `ED3D12BarrierImplementationType` の重複禁止

**C++20 コンセプト:**
```cpp
concept CIsImplementationOfBase     // 具象実装の基底チェック
concept CD3D12BarriersForAdapterImpl // Adapter実装 or nullptr_t
concept CD3D12BarriersForContextImpl // Context実装 or nullptr_t
concept CD3D12BarriersFactoryEntry   // FactoryEntry テンプレートのインスタンスチェック
```

### 6.3.2 Windows プラットフォーム定義

`WindowsD3D12BarriersFactory.h`:

```cpp
using FD3D12BarriersFactory = TD3D12BarriersFactory<
#if D3D12RHI_SUPPORTS_ENHANCED_BARRIERS
    TD3D12BarriersFactoryEntry<
        ED3D12BarrierImplementationType::Enhanced,
        FD3D12EnhancedBarriersForAdapter,
        FD3D12EnhancedBarriersForContext>,
#endif
#if D3D12RHI_SUPPORTS_LEGACY_BARRIERS
    TD3D12BarriersFactoryEntry<
        ED3D12BarrierImplementationType::Legacy,
        FD3D12LegacyBarriersForAdapter,
        FD3D12LegacyBarriersForContext>,
#endif
    FNullD3D12BarriersFactoryEntry
>;
```

Windows では両方が有効なため、`BarriersForAdapterType` / `BarriersForContextType` は抽象インターフェース型となり、ランタイム仮想ディスパッチが発生する。


## 6.4 ID3D12BarriersForAdapter — Adapter レベルインターフェース

リソース作成とトランジション管理を担当する。

### 6.4.1 仮想 API 一覧

| メソッド | 説明 |
|----------|------|
| `ConfigureDevice(ID3D12Device*, bool)` | デバイス固有設定 (Legacy: FormatAliased/Discard, Enhanced: なし) |
| `GetTransitionDataSizeBytes()` | FRHITransition のプライベートデータサイズ |
| `GetTransitionDataAlignmentBytes()` | プライベートデータのアライメント |
| `CreateTransition(FRHITransition*, FRHITransitionCreateInfo)` | トランジションデータの構築 |
| `ReleaseTransition(FRHITransition*)` | トランジションデータの破棄 |
| `CreateCommittedResource(...)` | Committed Resource 作成 (初期状態設定付き) |
| `CreateReservedResource(...)` | Reserved Resource 作成 |
| `CreatePlacedResource(...)` | Placed Resource 作成 |
| `GetImplementationName()` | 実装名文字列 |

### 6.4.2 リソース作成における差異

**Legacy:** `D3D12_RESOURCE_STATES` を初期ステートとして `CreateCommittedResource` / `CreatePlacedResource` に渡す。

**Enhanced:** `D3D12_BARRIER_LAYOUT` を初期レイアウトとして `CreateCommittedResource3` / `CreatePlacedResource2` に渡す。

両方とも `D3D12RHI_SUPPORTS_UNCOMPRESSED_UAV` 有効時は Castable Formats 対応の API (CreateCommittedResource3, CreatePlacedResource2, CreateReservedResource2) を使用する。


## 6.5 ID3D12BarriersForContext — コンテキストレベルインターフェース

コマンドリスト記録中のバリア発行を担当する。

### 6.5.1 仮想 API 一覧

| メソッド | 説明 |
|----------|------|
| `BeginTransitions(Context, Transitions[])` | トランジション開始 (Split Barrier の前半) |
| `EndTransitions(Context, Transitions[])` | トランジション終了 (Split Barrier の後半) |
| `AddGlobalBarrier(Context, Before, After)` | グローバルバリア追加 |
| `AddBarrier(Context, Resource, Before, After, Sub)` | リソース個別バリア追加 |
| `FlushIntoCommandList(CmdList, TimestampAlloc)` | バッチ化バリアをコマンドリストにフラッシュ |
| `GetNumPendingBarriers()` | 未フラッシュバリア数取得 |


## 6.6 CVar 一覧

| CVar 名 | デフォルト | 説明 |
|----------|-----------|------|
| `d3d12.AllowDiscardResources` | `1` | Transient Aliasing Acquire 後の DiscardResource 呼出しを許可 |
| `d3d12.DisableDiscardOfDepthResources` | `0` | Depth リソースの Discard をスキップ |
| `d3d12.BatchResourceBarriers` | `1` | リソースバリアのバッチ化を許可 |


## 6.7 Legacy バリアシステム (D3D12_RESOURCE_STATES)

### 6.7.1 アーキテクチャ

```
FD3D12LegacyBarriersForContext
    |
    +--- FD3D12LegacyBarriersBatcher  (TArray<D3D12_RESOURCE_BARRIER>)
    |        |
    |        +--- AddTransition()   → D3D12_RESOURCE_BARRIER_TYPE_TRANSITION
    |        +--- AddUAV()          → D3D12_RESOURCE_BARRIER_TYPE_UAV
    |        +--- AddAliasingBarrier() → D3D12_RESOURCE_BARRIER_TYPE_ALIASING
    |        +--- FlushIntoCommandList() → ResourceBarrier() 一括発行
    |
    +--- FD3D12LegacyBarriersTransitionData  (FRHITransition 内部)
             |
             +--- TransitionInfos[]   リソースごとの遷移情報
             +--- AliasingInfos[]     Transient Aliasing 情報
             +--- AliasingOverlaps[]  Aliasing オーバーラップ情報
             +--- SyncPoints[]        クロスパイプラインフェンス
             +--- bCrossPipeline      パイプライン跨ぎフラグ
             +--- bAsyncToAllPipelines  AsyncCompute→All 特殊ケース
```

### 6.7.2 カスタムリソースステート

```cpp
#define D3D12_RESOURCE_STATE_TBD      D3D12_RESOURCE_STATES(-1 ^ (1 << 31))  // 未確定 (後で解決)
#define D3D12_RESOURCE_STATE_CORRUPT  D3D12_RESOURCE_STATES(-2 ^ (1 << 31))  // 破損状態
```

### 6.7.3 ED3D12Access → D3D12_RESOURCE_STATES マッピング

`GetD3D12ResourceState()` が `ED3D12Access` を `D3D12_RESOURCE_STATES` に変換する。

| ED3D12Access | D3D12_RESOURCE_STATES |
|--------------|------------------------|
| `Common` | `COMMON` |
| `RTV` | `RENDER_TARGET` |
| `UAVMask / UAVCompute / UAVGraphics` | `UNORDERED_ACCESS` |
| `DSVWrite` | `DEPTH_WRITE` |
| `DSVRead` | `DEPTH_READ` |
| `CopyDest` | `COPY_DEST` |
| `CopySrc` | `COPY_SOURCE` |
| `ResolveDst` | `RESOLVE_DEST` |
| `ResolveSrc` | `RESOLVE_SOURCE` |
| `Present` | `PRESENT` |
| `GenericRead / ReadOnlyMask` | `GENERIC_READ` |
| `SRVGraphics` | `PIXEL_SHADER_RESOURCE \| NON_PIXEL_SHADER_RESOURCE` |
| `SRVCompute` | `NON_PIXEL_SHADER_RESOURCE` |
| `VertexOrIndexBuffer` | `VERTEX_AND_CONSTANT_BUFFER \| INDEX_BUFFER` |
| `BVHRead / BVHWrite` | `RAYTRACING_ACCELERATION_STRUCTURE` |
| `Discard` | リソースフラグに基づき動的決定 |

**Discard ステートの決定ロジック (`GetDiscardedResourceState`):**
- `ALLOW_RENDER_TARGET` + Direct Queue → `RENDER_TARGET`
- `ALLOW_DEPTH_STENCIL` + Direct Queue → `DEPTH_WRITE`
- `ALLOW_UNORDERED_ACCESS` → `UNORDERED_ACCESS`
- それ以外 → `COMMON`

### 6.7.4 FD3D12LegacyBarriersBatcher

バッチ化バリアの蓄積とフラッシュを担当する。

**主要最適化 — 逆方向トランジションの相殺:**
```cpp
// AddTransition() 内: 最後のバリアの逆遷移なら両方を除去
if (Last.pResource == pResource->GetResource()
    && Subresource == Last.Subresource
    && Before == Last.StateAfter
    && After  == Last.StateBefore)
{
    Barriers.RemoveAt(Barriers.Num() - 1);
    return -1;  // バリア数がマイナスに
}
```

**アイドル時間タイムスタンプ:**
バックバッファへの書込みトランジション (`RENDER_TARGET`, `UNORDERED_ACCESS`, `COPY_DEST`, `RESOLVE_DEST`, `STREAM_OUT`) は `BarrierFlag_CountAsIdleTime` でマーキングされ、FlushIntoCommandList 時にアイドルタイムスタンプクエリ (`IdleBegin`/`IdleEnd`) で囲まれる。これによりスワップチェーン待機時間をGPUプロファイリングから除外できる。

**フラッシュ処理:**
同一アイドルフラグのバリアをバッチ化し、`CommandList->ResourceBarrier()` を一括呼出し。

### 6.7.5 トランジション処理フロー

```
RHI から FRHITransition が作成される
    |
    v
CreateTransition() で FD3D12LegacyBarriersTransitionData を構築
    |  - TransitionInfos / AliasingInfos をコピー
    |  - クロスパイプライン時は FD3D12SyncPoint を作成
    |  - bAsyncToAllPipelines 特殊ケース検出
    v
BeginTransitions() / EndTransitions()
    |  - ShouldProcessTransition() でどちらのフェーズで処理するか判定
    |  - HandleReservedResourceCommits() — Reserved Resource のコミット/デコミット
    |  - HandleDiscardResources() — Discard トランジション処理
    |  - HandleTransientAliasing() — Aliasing バリア発行
    |  - HandleResourceTransitions() — ステート遷移バリア発行
    v
FlushIntoCommandList() → CommandList->ResourceBarrier()
```

**クロスパイプライン処理の判定:**
- AsyncCompute → Graphics + All の場合: `bAsyncToAllPipelines = true`
  - Begin: AsyncCompute パイプラインで処理
  - End: Graphics パイプラインで処理
- それ以外: `ProcessTransitionDuringBegin()` の論理で Begin/End を決定

### 6.7.6 ConfigureDevice の設定

Legacy バリアモード選択時、以下のグローバル設定が適用される:
```cpp
FD3D12DynamicRHI::SetFormatAliasedTexturesMustBeCreatedUsingCommonLayout(true);
GRHIGlobals.NeedsTransientDiscardStateTracking = true;
GRHIGlobals.NeedsTransientDiscardOnGraphicsWorkaround = true;
```

### 6.7.7 サブリソース列挙

`EnumerateSubresources()` が `FRHITransitionInfo` の Mip/Array/Plane 指定に基づきサブリソースインデックスを列挙する:
```cpp
Subresource = D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, MipCount, ArraySize);
```

全サブリソース指定 (`IsWholeResource()`) の場合は `D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES` を使用。


## 6.8 Enhanced バリアシステム (D3D12_BARRIER_LAYOUT)

### 6.8.1 アーキテクチャ

Enhanced Barriers は D3D12 Device Interface 11 以降で使用可能な新しいバリアモデルで、リソースの同期 (Sync)、アクセス (Access)、レイアウト (Layout) を個別に指定する。

```
FD3D12EnhancedBarriersForContext
    |
    +--- FD3D12EnhancedBarriersBatcher
    |        |
    |        +--- TArray<D3D12_GLOBAL_BARRIER>   GlobalBarriers
    |        +--- TArray<D3D12_TEXTURE_BARRIER>  TextureBarriers
    |        +--- TArray<D3D12_BUFFER_BARRIER>   BufferBarriers
    |        +--- TArray<FD3D12BarrierGroup>     BarrierGroups (メタデータ)
    |        |
    |        +--- FlushIntoCommandList() → Barrier() 一括発行
    |
    +--- FD3D12EnhancedBarriersTransitionData  (FRHITransition 内部)
             |
             +--- TransitionInfos[]
             +--- SyncPoints[]
             +--- SrcPipelines / DstPipelines
             +--- CreateFlags
```

### 6.8.2 3次元バリアモデル

Enhanced Barriers は3つの独立した次元でバリアを記述する:

```
FD3D12BarrierValues
{
    D3D12_BARRIER_SYNC   Sync;    // GPU パイプラインステージ同期
    D3D12_BARRIER_ACCESS Access;  // メモリアクセスタイプ
    D3D12_BARRIER_LAYOUT Layout;  // テクスチャメモリレイアウト (バッファには不要)
};
```

### 6.8.3 キュー互換性テーブル

Enhanced Barriers では各パイプラインキューごとに使用可能な Sync / Access / Layout の組合せが制限される。

**Sync 互換性:**

| キュー | 固有の Sync ビット |
|--------|-------------------|
| Direct | `DRAW`, `INDEX_INPUT`, `VERTEX_SHADING`, `PIXEL_SHADING`, `DEPTH_STENCIL`, `RENDER_TARGET`, `RESOLVE`, `PREDICATION` |
| Compute | `SPLIT` |
| 共通 | `ALL`, `COMPUTE_SHADING`, `RAYTRACING`, `COPY`, `EXECUTE_INDIRECT`, `ALL_SHADING`, `NON_PIXEL_SHADING`, `BUILD/COPY_RAYTRACING_*`, `CLEAR_UAV`, `EMIT_RT_POSTBUILD_INFO` |

**Layout 互換性:**

| キュー | Layout 一覧 |
|--------|------------|
| Direct (19個) | `COMMON`, `GENERIC_READ`, `RENDER_TARGET`, `UAV`, `DSV_WRITE/READ`, `SRV`, `COPY_SRC/DEST`, `RESOLVE_SRC/DEST`, `SHADING_RATE_SOURCE`, `DIRECT_QUEUE_*` (6種), `UNDEFINED` |
| Compute (14個) | `COMMON`, `GENERIC_READ`, `UAV`, `SRV`, `COPY_SRC/DEST`, `COMPUTE_QUEUE_*` (6種), `UNDEFINED`, + HACK: `DIRECT_QUEUE_GENERIC_READ` |
| 共通 | DirectQueueCompatible ∩ ComputeQueueCompatible |

**キュー固有レイアウトの汎化:**
`GetQueueAgnosticVersionOfLayout()` は `DIRECT_QUEUE_*` / `COMPUTE_QUEUE_*` プレフィクスのレイアウトを対応する汎用レイアウトに変換する:
```cpp
DIRECT_QUEUE_COMMON  → COMMON
COMPUTE_QUEUE_COMMON → COMMON
DIRECT_QUEUE_GENERIC_READ → GENERIC_READ
// ... 同様に COPY_SOURCE, COPY_DEST, UAV, SRV
```

### 6.8.4 ED3D12Access → Enhanced Barriers 変換

3つの変換関数が `ED3D12Access` を Enhanced Barriers の3次元に変換する:

**GetEBSync(ED3D12Access, ERHIPipeline) → D3D12_BARRIER_SYNC:**

| ED3D12Access | D3D12_BARRIER_SYNC |
|--------------|-------------------|
| `CPURead` | `NONE` |
| `Present` | `ALL` (Windows) / `NONE` (その他) |
| `IndirectArgs` | `EXECUTE_INDIRECT` |
| `VertexOrIndexBuffer` | `VERTEX_SHADING \| INDEX_INPUT \| ALL_SHADING` |
| `SRVCompute` | `COMPUTE_SHADING` |
| `SRVGraphicsPixel` | `PIXEL_SHADING` |
| `SRVGraphicsNonPixel` | `NON_PIXEL_SHADING` |
| `UAVCompute` | `COMPUTE_SHADING` |
| `UAVGraphics` | `VERTEX_SHADING \| PIXEL_SHADING` |
| `RTV` | `RENDER_TARGET` |
| `DSVRead / DSVWrite` | `DEPTH_STENCIL` |
| `CopySrc / CopyDest` | `COPY` |
| `ResolveSrc / ResolveDst` | `RESOLVE` |
| `BVHRead` | `RAYTRACING \| BUILD_RT_AS \| COPY_RT_AS \| EMIT_RT_POSTBUILD_INFO` |
| `BVHWrite` | `BUILD_RT_AS \| COPY_RT_AS` (+ Windows 以外は `RAYTRACING`) |
| `ShadingRateSource` | `PIXEL_SHADING` |
| `Common / GenericRead / Unknown` | `ALL` |
| `Discard` | `NONE` |

**GetEBAccess(ED3D12Access, ERHIPipeline) → D3D12_BARRIER_ACCESS:**

| ED3D12Access | D3D12_BARRIER_ACCESS |
|--------------|---------------------|
| `CPURead` | `COMMON` |
| `Present` | `COMMON` |
| `IndirectArgs` | `INDIRECT_ARGUMENT` |
| `VertexOrIndexBuffer` | `VERTEX_BUFFER \| INDEX_BUFFER \| CONSTANT_BUFFER` |
| `SRVMask` | `SHADER_RESOURCE` |
| `UAVMask` | `UNORDERED_ACCESS` |
| `RTV` | `RENDER_TARGET` |
| `DSVRead` | `DEPTH_STENCIL_READ` |
| `DSVWrite` | `DEPTH_STENCIL_WRITE` (READ ビットはハック除去) |
| `CopySrc` | `COPY_SOURCE` |
| `CopyDest` | `COPY_DEST` |
| `ResolveSrc` | `RESOLVE_SOURCE` |
| `ResolveDst` | `RESOLVE_DEST` |
| `BVHRead` | `RAYTRACING_ACCELERATION_STRUCTURE_READ` |
| `BVHWrite` | `RAYTRACING_ACCELERATION_STRUCTURE_WRITE` |
| `ShadingRateSource` | `SHADING_RATE_SOURCE` |
| `Common / GenericRead / Unknown` | `COMMON` (早期リターン) |
| `Discard` | `NO_ACCESS` (早期リターン) |

**GetEBLayout(ED3D12Access, ERHIPipeline, FD3D12Texture*) → D3D12_BARRIER_LAYOUT:**

パイプライン (Graphics / AsyncCompute / All) に応じてキュー固有レイアウトを選択:
- 例: `CopySrc` + Graphics → `DIRECT_QUEUE_COPY_SOURCE`
- 例: `CopySrc` + AsyncCompute → `COMPUTE_QUEUE_COPY_SOURCE`
- 例: `CopySrc` + All → `COPY_SOURCE`

特殊ケース:
- `Discard` → `UNDEFINED`
- `Present` → `PRESENT`
- `DSVRead | DSVWrite` → `DEPTH_STENCIL_WRITE`
- `SRVMask` + `SkipsFastClearFinalize()` → Layout に FCE フラグを追加

### 6.8.5 FD3D12EnhancedBarriersBatcher

Enhanced Barriers 版のバッチャーは3種類のバリア配列を個別に管理する。

```
FD3D12EnhancedBarriersBatcher
    |
    +--- GlobalBarriers[]    ← D3D12_GLOBAL_BARRIER
    +--- TextureBarriers[]   ← D3D12_TEXTURE_BARRIER (Layout 遷移付き)
    +--- BufferBarriers[]    ← D3D12_BUFFER_BARRIER
    +--- BarrierGroups[]     ← D3D12_BARRIER_GROUP (メタ情報: Type, Num, ポインタ)
```

**型安全テンプレート機構:**
```cpp
template <D3D12_BARRIER_TYPE TBarrierType>
static auto GetMemberArrayForBarrierType();
// D3D12_BARRIER_TYPE_GLOBAL  → &GlobalBarriers
// D3D12_BARRIER_TYPE_TEXTURE → &TextureBarriers
// D3D12_BARRIER_TYPE_BUFFER  → &BufferBarriers
```

**AddBarriers<T>():**
- `CheckBarrierValuesAreCompatible()` で Sync/Access/Layout のキュー互換性を検証
- 同一タイプ・同一アイドルフラグの連続バリアは既存の BarrierGroup にマージ
- アイドルタイムマーキングは `BarrierType_CountAsIdleTime` ビットを `D3D12_BARRIER_TYPE` の上位ビットに格納

**FlushIntoCommandList():**
`ID3D12GraphicsCommandList7::Barrier()` API を使用してバリアグループを一括発行。Legacy の `ResourceBarrier()` とは異なり、複数タイプのバリアを1回の API 呼出しでサブミットできる。

### 6.8.6 バリア破棄条件

`BarrierCanBeDiscarded()` は不要なバリアを除去する:
```cpp
bool BarrierCanBeDiscarded(Before, After)
{
    return (Before.Sync   == After.Sync)
        && (Before.Access == After.Access)
        && (Before.Layout == After.Layout)
        // ComputeWrite → ComputeWrite は各コンピュートユニットの
        // キャッシュ独立性のためスキップ不可
        && !SyncAndAccessAreComputeWrite(Before.Sync, Before.Access);
}
```

### 6.8.7 AccessCompatibleSync テーブル

DX 仕様から直接コピーされた23要素の静的テーブル (`TStaticArray<D3D12_BARRIER_SYNC, 23>`) が、各 `D3D12_BARRIER_ACCESS` ビットに対して互換性のある `D3D12_BARRIER_SYNC` フラグを定義する。

`GetAccessCompatibleSync()` はアクセスフラグのセットビットを走査し、対応する互換 Sync をOR合成する:
```cpp
static constexpr D3D12_BARRIER_SYNC GetAccessCompatibleSync(D3D12_BARRIER_ACCESS InAccess)
{
    // std::countl_zero でビット走査、AccessCompatibleSync[] テーブル参照
}
```

### 6.8.8 LayoutCompatibleAccess テーブル

32要素の静的テーブル (`TStaticArray<D3D12_BARRIER_ACCESS, 32>`) が、各 `D3D12_BARRIER_LAYOUT` に対して互換性のある `D3D12_BARRIER_ACCESS` を定義する。

例:
- `RENDER_TARGET` レイアウト → `RENDER_TARGET` アクセスのみ
- `COMMON` レイアウト → `SHADER_RESOURCE | COPY_DEST | COPY_SOURCE`
- `DEPTH_STENCIL_WRITE` レイアウト → `DEPTH_STENCIL_READ | DEPTH_STENCIL_WRITE`

### 6.8.9 コンパイル時配列操作

Enhanced Barriers の実装は高度な `consteval` テンプレートメタプログラミングを使用して、キュー互換性テーブルのコンパイル時フィルタリングと集合演算を行う:

```cpp
// 配列 A と B の交差集合を取得
template <TStaticArray InArrayA, TStaticArray InArrayB>
static consteval auto GetIntersectionOf();

// 配列 A にのみ存在する要素を取得
template <TStaticArray InArrayA, TStaticArray InArrayB>
static consteval auto GetElementsExclusiveToA();
```

これにより以下のテーブルがコンパイル時に生成される:
- `AllQueueCompatibleLayouts` = Direct ∩ Compute
- `DirectQueueSpecificLayouts` = Direct - Compute
- `ComputeQueueSpecificLayouts` = Compute - Direct

### 6.8.10 プラットフォーム固有ワークアラウンド

Enhanced Barriers にはいくつかの D3D12 仕様/バリデーション層の問題に対するワークアラウンドが含まれる:

| マクロ | 値 | 説明 |
|--------|-----|------|
| `PLATFORM_REQUIRES_ENHANCED_BARRIERS_GFX_ONLY_READ_BITS_HACK` | `1` | 複数パイプからの同時読取り時に GFX 専用読取りビット (`DSVRead`, `ShadingRateSource`, `ResolveSource`) を Compute キューでも許可 |
| `PLATFORM_REQUIRES_SYNC_RAYTRACING_NOT_COMPATIBLE_WITH_ACCESS_AS_WRITE` | Windows: `1` | `D3D12_BARRIER_SYNC_RAYTRACING` と `ACCESS_AS_WRITE` の組合せを除外 |
| `PLATFORM_REQUIRES_LAYOUT_DEPTH_STENCIL_WRITE_NOT_COMPATIBLE_WITH_ACCESS_DEPTH_STENCIL_READ` | `1` | `DSV_WRITE` レイアウト設定時に `DEPTH_STENCIL_READ` アクセスビットを除去 |

各プラットフォームは `GetSkipFastClearEliminateLayoutFlags()` を提供する必要がある (Fast Clear Eliminate のスキップ用レイアウトフラグ)。


## 6.9 実装パターンの分離 (Impl namespace)

Diamond Inheritance 問題を回避するため、実装ロジックは namespace 関数として分離される:

```
FD3D12EnhancedBarriersForAdapterImpl (namespace)
    +--- GetInitialLayout()
    +--- ConfigureDevice()
    +--- CreateTransition() / ReleaseTransition()
    +--- CreateCommittedResource() / CreatePlacedResource() / CreateReservedResource()

FD3D12EnhancedBarriersForAdapter (class, final)
    +--- ID3D12BarriersForAdapter を実装
    +--- 各 virtual メソッドが namespace 関数に委譲
```

Legacy 側も同様のパターン (`FD3D12LegacyBarriersForAdapterImpl` namespace)。
この設計により、他プラットフォームがインターフェースと実装の両方を特殊化する際に、多重継承の問題を回避できる。


## 6.10 Legacy リソース作成パス

### 6.10.1 Intel Extensions 対応

`INTEL_EXTENSIONS` 有効時、64bit Atomic エミュレーションが必要なリソースは `INTC_D3D12_CreateCommittedResource` / `INTC_D3D12_CreatePlacedResource` を使用する。

### 6.10.2 Uncompressed UAV 対応

`D3D12RHI_SUPPORTS_UNCOMPRESSED_UAV` 有効時:
- `CreateCommittedResource3` + `CD3DX12_RESOURCE_DESC1` + Castable Formats
- `CreatePlacedResource2` + 初期レイアウト `D3D12_BARRIER_LAYOUT_COMMON`
- `CreateReservedResource2` + Castable Formats
- 初期ステートは `D3D12_RESOURCE_STATE_COMMON` であることを `checkf` で検証

### 6.10.3 標準パス

上記条件に該当しない場合、標準の `CreateCommittedResource` / `CreatePlacedResource` / `CreateReservedResource` API を使用する。


## 6.11 デバッグとロギング

### 6.11.1 Legacy バリアログ

`DEBUG_RESOURCE_STATES` 有効時、`LogResourceBarriers()` がバリアの詳細をログ出力する:
- `TRANSITION`: リソース名、ステート遷移、サブリソースインデックス
- `UAV`: リソース名
- `ALIASING`: Before/After リソース名

### 6.11.2 Enhanced バリアログ

2つのコンパイル時フラグで制御:
- `D3D12_ENHANCED_BARRIERS_LOG_BARRIERS_WHEN_BATCHED` — バッチ追加時にログ
- `D3D12_ENHANCED_BARRIERS_LOG_BARRIERS_WHEN_FLUSHED` — フラッシュ時にログ

ログ形式:
```
D3D12 Texture Barrier
  |   Resource: <name> (0x<ptr>)
  | SyncBefore: <flags> (0x<hex>)
  |  SyncAfter: <flags> (0x<hex>)
  |AccessBefore: <flags> (0x<hex>)
  | AccessAfter: <flags> (0x<hex>)
  |LayoutBefore: <enum> (0x<hex>)
  | LayoutAfter: <enum> (0x<hex>)
  |  Subresource: <n>
  |      Discard: <0/1>
```

フラグ→文字列変換には `ConvertFlagsToString()` (ビットフラグ用) と `ConvertEnumToString()` (単一値用) テンプレートが使用される。


## 6.12 クラス関係図

```
                        ID3D12BarriersForAdapter
                               ^       ^
                               |       |
     FD3D12EnhancedBarriersForAdapter  FD3D12LegacyBarriersForAdapter
            (delegates to)              (delegates to)
     FD3D12EnhancedBarriersFor-         FD3D12LegacyBarriersFor-
          AdapterImpl (ns)                   AdapterImpl (ns)

                        ID3D12BarriersForContext
                               ^       ^
                               |       |
     FD3D12EnhancedBarriersForContext  FD3D12LegacyBarriersForContext
            |                                    |
     FD3D12EnhancedBarriersBatcher       FD3D12LegacyBarriersBatcher
     (Global/Texture/Buffer 分離)        (D3D12_RESOURCE_BARRIER 統一)
     → Barrier() API                    → ResourceBarrier() API

                    TD3D12BarriersFactory<...>
                               |
                    FD3D12BarriersFactory (platform typedef)
                               |
                    CreateBarriersForAdapter()
                    CreateBarriersForContext()
```

**ソースファイル構成:**

| ファイル | 行数 | 内容 |
|----------|------|------|
| `ID3D12Barriers.h` | 125 | 抽象インターフェース、CVar 定義 |
| `D3D12BarriersFactory.h` | 266 | テンプレートファクトリ、C++20 コンセプト |
| `D3D12Access.h` | 81 | ED3D12Access 列挙 |
| `D3D12EnhancedBarriers.h` | 187 | Enhanced Barriers ヘッダ |
| `D3D12EnhancedBarriers.cpp` | ~3200 | Enhanced Barriers 実装 (テーブル、変換、Batcher) |
| `D3D12LegacyBarriers.h` | 186 | Legacy Barriers ヘッダ |
| `D3D12LegacyBarriers.cpp` | ~1700 | Legacy Barriers 実装 (ステート変換、Batcher) |
| `WindowsD3D12BarriersFactory.h` | 24 | Windows プラットフォーム Factory typedef |
