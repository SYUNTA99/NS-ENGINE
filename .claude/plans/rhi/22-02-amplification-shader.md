# 22-02: 増幅シェーダー

## 概要

メッシュシェーダーの起動を制御する増幅（タスク/アンプリフィケーション）シェーダー、
LOD選択、カリング、動的インスタンシングを実行、

## ファイル

- `Source/Engine/RHI/Public/RHIAmplificationShader.h`

## 依存

- 22-01-mesh-shader.md (IRHIMeshShader)
- 06-02-shader-interface.md (IRHIShader)

## 定義

```cpp
namespace NS::RHI
{

/// 増幅シェーダー出力ペイロード最大サイズ
constexpr uint32 kRHIMaxAmplificationPayloadSize = 16 * 1024; // 16KB

/// 増幅シェーダー記述
struct RHIAmplificationShaderDesc
{
    const void* bytecode = nullptr;
    uint64 bytecodeSize = 0;
    uint32 payloadSize = 0;         ///< メッシュシェーダーへ渡すペイロードサイズ
    const char* entryPoint = "main";
    const char* debugName = nullptr;
};

/// 増幅シェーダーディスパッチ情報
struct RHIAmplificationDispatchInfo
{
    uint32 meshShaderGroupsX = 0;   ///< メッシュシェーダーグループ数X
    uint32 meshShaderGroupsY = 1;   ///< メッシュシェーダーグループ数Y
    uint32 meshShaderGroupsZ = 1;   ///< メッシュシェーダーグループ数Z
};

// ════════════════════════════════════════════════════════════════
// ペイロード定義
// ════════════════════════════════════════════════════════════════

/// LOD選択ペイロード例
struct RHILODSelectionPayload
{
    uint32 meshletOffset;       ///< 選択されたLODのメッシュレットオフセット
    uint32 meshletCount;        ///< 選択されたLODのメッシュレット数
    uint32 lodLevel;            ///< LODレベル
    uint32 objectId;            ///< オブジェクトID
};

/// インスタンスカリングペイロード例
struct RHIInstanceCullingPayload
{
    uint32 visibleInstanceIndices[64];  ///< 可視インスタンスインデックス
    uint32 visibleCount;                 ///< 可視インスタンス数
};

// ════════════════════════════════════════════════════════════════
// 増幅のメッシュシェーダー連携
// ════════════════════════════════════════════════════════════════

/// 増幅メッシュシェーダーペア
class RHIAmplificationMeshPipeline
{
public:
    RHIAmplificationMeshPipeline(
        RHIAmplificationShaderRef amplificationShader,
        RHIMeshShaderRef meshShader)
        : m_amplificationShader(std::move(amplificationShader))
        , m_meshShader(std::move(meshShader))
    {
    }

    IRHIAmplificationShader* GetAmplificationShader() const
    {
        return m_amplificationShader.Get();
    }

    IRHIMeshShader* GetMeshShader() const
    {
        return m_meshShader.Get();
    }

    /// ペイロードサイズの互換性チェック
    bool ValidatePayloadCompatibility() const
    {
        return m_amplificationShader->GetPayloadSize() > 0;
    }

private:
    RHIAmplificationShaderRef m_amplificationShader;
    RHIMeshShaderRef m_meshShader;
};

} // namespace NS::RHI
```

## HLSL例

```hlsl
// 増幅シェーダー例：LOD選択とカリング

struct LODPayload
{
    uint meshletOffset;
    uint meshletCount;
    uint lodLevel;
    uint objectId;
};

groupshared LODPayload s_payload;

[numthreads(1, 1, 1)]
void AmplificationMain(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    // オブジェクトデータ取得
    ObjectData obj = objects[gid];

    // フラスタムカリング
    if (!IsInFrustum(obj.boundingSphere, frustumPlanes))
    {
        // カリングされたオブジェクトのメッシュシェーダーを起動しない
        return;
    }

    // LOD選択（距離ベース）。
    float distance = length(obj.position - cameraPosition);
    uint lodLevel = SelectLOD(distance, obj.lodDistances);

    // ペイロード設定
    s_payload.meshletOffset = obj.lodMeshletOffsets[lodLevel];
    s_payload.meshletCount = obj.lodMeshletCounts[lodLevel];
    s_payload.lodLevel = lodLevel;
    s_payload.objectId = gid;

    // メッシュシェーダーをディスパッチ
    // 各メッシュレットに対して1グループ
    uint meshGroupCount = (s_payload.meshletCount + 31) / 32;
    DispatchMesh(meshGroupCount, 1, 1, s_payload);
}

// メッシュシェーダー側でのペイロード受信
[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void MeshMain(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload LODPayload payload,  // 増幅シェーダーからのペイロード
    out vertices MeshOutput verts[64],
    out indices uint3 tris[126])
{
    uint meshletIndex = payload.meshletOffset + gid;
    if (meshletIndex >= payload.meshletOffset + payload.meshletCount)
        return;

    // メッシュレット処理..
}
```

## 使用パターン

### 1. LOD選択
```
増幅シェーダー（1スレッド/オブジェクト）
    ↔ LOD計算、ペイロードにメッシュレット範囲を設定
メッシュシェーダー（Nグループ、メッシュレット）。
```

### 2. GPUカリング
```
増幅シェーダー（Nグループ、オブジェクトグループ）
    ↔ フラスタム+オクルージョンカリング
    ↔ 可視オブジェクトのみDispatchMesh
メッシュシェーダー
```

### 3. 動的インスタンシング
```
増幅シェーダー
    ↔ インスタンス可視性テスト
    ↔ 可視インスタンスIDをペイロードに格納
メッシュシェーダー（インスタンス変換を適用）。
```

## 検証

- [ ] ペイロードサイズ制限
- [ ] DispatchMeshグループ数計算
- [ ] カリング時の早期リターン
- [ ] メッシュシェーダーとのペイロード受け渡し
