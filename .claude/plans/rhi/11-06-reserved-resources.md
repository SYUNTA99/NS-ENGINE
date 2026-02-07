# 11-06: Reserved/Sparse Resources

## 目的

物理メモリを事前に割り当てずにリソースを作成するReserved Resourceシステムを定義する。

## 参照ドキュメント

- .claude/plans/rhi/docs/RHI_Implementation_Guide_Part10.md (1. Reserved/Sparse Resources)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIReservedResource.h`

既存ファイル修正:
- `Source/Engine/RHI/Public/RHIEnums.h` (ERHIBufferFlags, ERHITextureFlags拡張)

## Reserved Resourceの概念

```
通常リソース:
┌─────────────────────────────────────────────────────────────│
│仮想アドレス空間                                             │
│┌─────────────────────────────────────────────────────────││
││物理メモリ (全体が割り当て済み)                          ││
││[████████████████████████████████████████████████████] ││
│└─────────────────────────────────────────────────────────││
└─────────────────────────────────────────────────────────────│

Reserved Resource:
┌─────────────────────────────────────────────────────────────│
│仮想アドレス空間（大きく確保）                                 │
│┌─────────────────────────────────────────────────────────││
││物理メモリ (必要な部分のみ割り当て)                      ││
││[████████]          [████]               [██████]      ││
││ コミット済み         コミット済み          コミット済み    ││
│└─────────────────────────────────────────────────────────││
└─────────────────────────────────────────────────────────────│

タイル単位(65536 bytes = 64KB) でコミット/デコミット可能
```

## TODO

### ケイパビリティ

```cpp
/// Reserved Resourceケイパビリティ
struct RHIReservedResourceCapabilities {
    /// バッファでのReserved Resourceサポート
    bool supportsBuffers = false;

    /// 2Dテクスチャでのサポート
    bool supportsTexture2D = false;

    /// 3Dテクスチャ (ボリューム) でのサポート
    bool supportsTexture3D = false;

    /// ミップマップ付きテクスチャでのサポート
    bool supportsMipmaps = false;

    /// タイルサイズ (バイト
    static constexpr uint32 kTileSizeInBytes = 65536;  // 64KB

    /// 最大仮想サイズ
    uint64 maxVirtualSize = 0;
};

/// ケイパビリティを取得(IRHIDeviceに追加)
virtual const RHIReservedResourceCapabilities& GetReservedResourceCapabilities() const = 0;
```

### リソース作成フラグ

```cpp
/// バッファフラグ拡張
enum class ERHIBufferFlags : uint32 {
    // ... 既存フラグ ...

    /// Reserved Resource (仮想アドレスのみ予約
    ReservedResource = 1 << 16,
};

/// テクスチャフラグ拡張
enum class ERHITextureFlags : uint32 {
    // ... 既存フラグ ...

    /// Reserved Resource
    ReservedResource = 1 << 20,

    /// 作成時に即座にコミット(Reserved Resourceと併用)
    ImmediateCommit = 1 << 21,
};
```

### コミット情報

```cpp
/// コミット操作記述
struct RHICommitResourceInfo {
    /// コミットするサイズ (バイト
    /// タイル境界 (64KB) に切り上げられる
    uint64 sizeInBytes;

    explicit RHICommitResourceInfo(uint64 inSize)
        : sizeInBytes(inSize) {}
};

/// コミット領域 (テクスチャ用)
struct RHITextureCommitRegion {
    /// ミップレベル
    uint32 mipLevel;

    /// 配列スライス
    uint32 arraySlice;

    /// 領域オフセット (タイル単位
    uint32 tileOffsetX;
    uint32 tileOffsetY;
    uint32 tileOffsetZ;

    /// 領域サイズ (タイル単位
    uint32 tileSizeX;
    uint32 tileSizeY;
    uint32 tileSizeZ;
};
```

### リソースインターフェース拡張

```cpp
/// Reserved Resource情報を取得
class IRHIResource {
    // ... 既存メソッド...

    /// Reserved Resourceか
    virtual bool IsReservedResource() const { return false; }

    /// 現在のコミットサイズ取得(バイト
    virtual uint64 GetCommittedSize() const { return 0; }

    /// 最大仮想サイズ取得(バイト
    virtual uint64 GetVirtualSize() const { return 0; }
};
```

### コマンドコンテキスト拡張

```cpp
class IRHICommandContext {
    // ... 既存メソッド...

    //=========================================================================
    // Reserved Resource 操作
    //=========================================================================

    /// バッファのコミットサイズ変更
    /// @param buffer 対象バッファ
    /// @param newCommitSize 新しいコミットサイズ
    /// @note 増加時の末尾にメモリ追加、減少時は末尾から解放
    /// @note 新しくコミットされた領域の内容は未定義
    virtual void CommitBuffer(
        IRHIBuffer* buffer,
        uint64 newCommitSize
    ) = 0;

    /// テクスチャ領域のコミッチ
    /// @param texture 対象テクスチャ
    /// @param regions コミットする領域配列
    /// @param regionCount 領域数
    /// @param commit true=コミット、false=デコミット
    virtual void CommitTextureRegions(
        IRHITexture* texture,
        const RHITextureCommitRegion* regions,
        uint32 regionCount,
        bool commit
    ) = 0;
};
```

### 遷移時のコミッチ

```cpp
/// リソース遷移にコミット操作を含める
struct RHITransitionInfo {
    // ... 既存メソッド...

    /// コミット情報 (オプション)
    /// 指定時は遷移と同時にコミットサイズを変更
    TOptional<RHICommitResourceInfo> commitInfo;
};

// 使用例
RHITransitionInfo transition;
transition.resource = reservedBuffer;
transition.accessBefore = ERHIAccess::Unknown;
transition.accessAfter = ERHIAccess::SRVCompute;
transition.commitInfo = RHICommitResourceInfo(newSize);

context->Transition(&transition, 1);
```

### タイルマッピング情報

```cpp
/// テクスチャタイル情報
struct RHITextureTileInfo {
    /// 各MIPレベルのタイル数
    struct MipInfo {
        uint32 tilesX;
        uint32 tilesY;
        uint32 tilesZ;
        uint32 totalTiles;
    };

    /// ミップ情報配列
    TArray<MipInfo> mipInfos;

    /// パックドミップ (タイル未満のミップ群)
    struct {
        uint32 startMip;        // パックドミップ開始レベル
        uint32 tileCount;       // 必要タイル数
    } packedMips;

    /// 総タイル数
    uint32 totalTiles;

    /// 総仮想サイズ (バイト
    uint64 totalVirtualSize;
};

/// タイル情報を取得(IRHIDeviceに追加)
virtual RHITextureTileInfo GetTextureTileInfo(
    const RHITextureDesc& desc
) const = 0;
```

### 使用パターン

**バッファ (動的サイズ):**
```cpp
// 1. 最大予想サイズでReserved Buffer作成
RHIBufferDesc desc;
desc.size = maxExpectedSize;  // 仮想サイズ
desc.flags = ERHIBufferFlags::ReservedResource | ERHIBufferFlags::ShaderResource;

auto buffer = device->CreateBuffer(desc);
// この時点では物理メモリ未割り当て

// 2. 必要に応じてコミッチ
uint64 currentSize = 1024 * 1024;  // 1MB
context->CommitBuffer(buffer, currentSize);

// 3. サイズ拡張
currentSize *= 2;  // 2MB
context->CommitBuffer(buffer, currentSize);
// SRV/UAV再作成不要。）。

// 4. サイズ縮小(メモリ解放)
currentSize /= 2;
context->CommitBuffer(buffer, currentSize);
```

**仮想テクスチャ:**
```cpp
// 1. 大きなReserved Texture作成
RHITextureDesc desc;
desc.width = 16384;
desc.height = 16384;
desc.mipLevels = 14;
desc.flags = ERHITextureFlags::ReservedResource;

auto texture = device->CreateTexture(desc);

// 2. タイル情報を取得
auto tileInfo = device->GetTextureTileInfo(desc);

// 3. 可視タイルのみコミッチ
TArray<RHITextureCommitRegion> visibleRegions;
for (auto& tile : visibleTiles) {
    visibleRegions.Add({
        .mipLevel = tile.mip,
        .tileOffsetX = tile.x,
        .tileOffsetY = tile.y,
        .tileSizeX = 1,
        .tileSizeY = 1,
        .tileSizeZ = 1
    });
}
context->CommitTextureRegions(texture, visibleRegions.GetData(),
    visibleRegions.Num(), true);

// 4. 不要タイルをデコミット
context->CommitTextureRegions(texture, oldRegions.GetData(),
    oldRegions.Num(), false);
```

### ユースケース

| ユースケース | 説明|
|-------------|------|
| 仮想テクスチャ | 巨大テクスチャの部分ロード|
| メガテクスチャ | 地形等の連続テクスチャ |
| スパースボクセル | 3Dデータの効率的格納|
| 動的バッファ | サイズ可変のGPUバッファ |
| ストリーミング | 大規模シーンの段階的ロード|

### プラットフォームサポート

```
┌─────────────────────────────────────────────────────────────│
│          Reserved Resource サポート状況                    │
├────────────────┬─────────┬───────────┬───────────┬─────────┤
│プラットフォーム │バッファ │2Dテクスチャ │3Dテクスチャ │ミップ  │
├────────────────┼─────────┼───────────┼───────────┼─────────┤
│D3D12 Tier 1   │✓      │✓        │✓        │✓      │
│D3D12 Tier 2   │✓      │✓        │✓        │✓      │
│D3D12 Tier 3   │✓      │✓        │✓        │✓      │
│Vulkan         │✓      │✓        │部分的    │✓      │
│Metal          │✓      │✓        │✓        │✓      │
└────────────────┴─────────┴───────────┴───────────┴─────────│
```

## 検証方法

- タイル境界でのコミット/デコミット
- SRV/UAV有効性の維持
- メモリ使用量の追跡
- アクセスパターンの正確性
- 未コミット領域アクセス時の動作

## 備考

Reserved Resourceは実験的機能であり、のプラットフォームでサポートされていい
使用前に必要ケイパビリティを確認すること、

新しくコミットされた領域の内容は未定義であるため、
使用前に必要なデータを書き込むか、クリアする必要がある、
