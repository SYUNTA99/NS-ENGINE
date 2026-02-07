# 22-01: メッシュシェーダー

## 概要

従来の頂点シェーダーパイプラインを置き換える、GPU駆動のメッシュ処理シェーダー、
頂点/インデックスバッファを使わず、メッシュレット単位で処理）。

## ファイル

- `Source/Engine/RHI/Public/RHIMeshShader.h`
- `Source/Engine/RHI/Public/IRHIMeshShader.h`

## 依存

- 06-02-shader-interface.md (IRHIShader基底
- 01-06-enums-shader.md (ERHIShaderStage)

## 定義

```cpp
namespace NS::RHI
{

// ════════════════════════════════════════════════════════════════
// 機能検出
// ════════════════════════════════════════════════════════════════

/// メッシュシェーダー機能フラグ
struct RHIMeshShaderCapabilities
{
    bool supported = false;                     ///< メッシュシェーダー対応
    bool amplificationShaderSupported = false;  ///< 増幅シェーダー対応

    uint32 maxOutputVertices = 0;       ///< メッシュシェーダー最大出力頂点数
    uint32 maxOutputPrimitives = 0;     ///< メッシュシェーダー最大出力プリミティブ数
    uint32 maxMeshWorkGroupSize = 0;    ///< メッシュシェーダーワークグループサイズ
    uint32 maxTaskWorkGroupSize = 0;    ///< 増幅シェーダーワークグループサイズ

    uint32 maxMeshOutputMemorySize = 0; ///< 出力メモリサイズ上限
    uint32 maxMeshSharedMemorySize = 0; ///< 共有メモリサイズ上限
    uint32 maxTaskOutputCount = 0;      ///< タスクシェーダー出力数上限
    uint32 maxTaskPayloadSize = 0;      ///< ペイロードサイズ上限

    bool prefersMeshShaderForLOD = false;           ///< LOD処理をMS推奨
    bool prefersMeshShaderForOcclusionCulling = false; ///< オクルージョンカリングにMS推奨
};

// ════════════════════════════════════════════════════════════════
// シェーダーインターフェース
// ════════════════════════════════════════════════════════════════

/// メッシュシェーダー
/// 頂点処理とプリミティブ生成を統合
class IRHIMeshShader : public IRHIShader
{
public:
    virtual ~IRHIMeshShader() = default;

    /// シェーダーステージ取得
    ERHIShaderStage GetStage() const override { return ERHIShaderStage::Mesh; }

    // ────────────────────────────────────────────────────────────
    // メッシュシェーダー固有情報
    // ────────────────────────────────────────────────────────────

    /// 出力トポロジー取得
    virtual ERHIPrimitiveTopology GetOutputTopology() const = 0;

    /// 最大出力頂点数
    virtual uint32 GetMaxOutputVertices() const = 0;

    /// 最大出力プリミティブ数
    virtual uint32 GetMaxOutputPrimitives() const = 0;

    /// スレッドグループサイズ取得
    virtual void GetThreadGroupSize(uint32& x, uint32& y, uint32& z) const = 0;
};

using RHIMeshShaderRef = TRefCountPtr<IRHIMeshShader>;

/// 増幅（タスク/アンプリフィケーション）シェーダー
/// メッシュシェーダーの起動を制御
class IRHIAmplificationShader : public IRHIShader
{
public:
    virtual ~IRHIAmplificationShader() = default;

    /// シェーダーステージ取得
    ERHIShaderStage GetStage() const override { return ERHIShaderStage::Amplification; }

    // ────────────────────────────────────────────────────────────
    // 増幅シェーダー固有情報
    // ────────────────────────────────────────────────────────────

    /// ペイロードサイズ取得（メッシュシェーダーへ渡すデータサイズ）。
    virtual uint32 GetPayloadSize() const = 0;

    /// スレッドグループサイズ取得
    virtual void GetThreadGroupSize(uint32& x, uint32& y, uint32& z) const = 0;
};

using RHIAmplificationShaderRef = TRefCountPtr<IRHIAmplificationShader>;

// ════════════════════════════════════════════════════════════════
// メッシュレット構造体
// ════════════════════════════════════════════════════════════════

/// メッシュレット定義
/// GPU側でも同じレイアウトを使用
struct RHIMeshlet
{
    uint32 vertexOffset;        ///< 頂点インデックス配列内のオフセット
    uint32 triangleOffset;      ///< プリミティブインデックス配列内のオフセット
    uint32 vertexCount;         ///< 頂点数（最大64または128）。
    uint32 triangleCount;       ///< 三角形数（最大64または128）。
};
static_assert(sizeof(RHIMeshlet) == 16, "Meshlet must be 16 bytes for GPU alignment");

/// メッシュレットバウンディング情報
struct RHIMeshletBounds
{
    float centerX, centerY, centerZ;    ///< バウンディング球の中心
    float radius;                        ///< バウンディング球の半径

    float coneAxisX, coneAxisY, coneAxisZ;  ///< 法線コーン軸
    float coneCutoff;                        ///< 法線コーンカットオフ
};
static_assert(sizeof(RHIMeshletBounds) == 32, "MeshletBounds must be 32 bytes");

/// メッシュレットデータセット
struct RHIMeshletData
{
    std::vector<RHIMeshlet> meshlets;           ///< メッシュレット配列
    std::vector<RHIMeshletBounds> bounds;       ///< バウンディング配列
    std::vector<uint32> vertexIndices;          ///< 頂点インデックス
    std::vector<uint8> primitiveIndices;        ///< プリミティブインデックス（バイト三角形）。

    uint32 GetMeshletCount() const { return static_cast<uint32>(meshlets.size()); }
};

// ════════════════════════════════════════════════════════════════
// メッシュシェーダー描画コマンド
// ════════════════════════════════════════════════════════════════

// IRHIGraphicsContextへの追加

class IRHIGraphicsContext
{
public:
    // ... 既存メソッド ...

    /// メッシュシェーダーディスパッチ
    /// @param groupCountX X方向グループ数
    /// @param groupCountY Y方向グループ数
    /// @param groupCountZ Z方向グループ数
    virtual void DispatchMesh(
        uint32 groupCountX,
        uint32 groupCountY = 1,
        uint32 groupCountZ = 1) = 0;

    /// 間接メッシュシェーダーディスパッチ
    /// @param argumentBuffer 引数バッファ（RHIDispatchMeshArguments）。
    /// @param argumentOffset バッファオフセット
    virtual void DispatchMeshIndirect(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset = 0) = 0;
};

} // namespace NS::RHI
```

## HLSL例

```hlsl
// メッシュシェーダー例
struct MeshOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct PrimitiveOutput
{
    uint primitiveId : SV_PrimitiveID;
};

[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void MeshMain(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out vertices MeshOutput verts[64],
    out indices uint3 tris[126],
    out primitives PrimitiveOutput prims[126])
{
    Meshlet m = meshlets[gid];

    SetMeshOutputCounts(m.vertexCount, m.triangleCount);

    if (gtid < m.vertexCount)
    {
        uint vertexIndex = vertexIndices[m.vertexOffset + gtid];
        Vertex v = vertices[vertexIndex];

        verts[gtid].position = mul(mvp, float4(v.position, 1.0));
        verts[gtid].normal = v.normal;
        verts[gtid].uv = v.uv;
    }

    if (gtid < m.triangleCount)
    {
        uint offset = m.triangleOffset + gtid * 3;
        tris[gtid] = uint3(
            primitiveIndices[offset],
            primitiveIndices[offset + 1],
            primitiveIndices[offset + 2]
        );
        prims[gtid].primitiveId = gid * 126 + gtid;
    }
}
```

## プラットフォームサポート

| API | メッシュシェーダー | 増幅シェーダー | 備考|
|-----|------------------|----------------|------|
| D3D12 | SM 6.5+ | Task Shader | Windows 10 20H1+ |
| Vulkan | VK_EXT_mesh_shader | Task Shader | 要拡張 |
| Metal | Object/Mesh | Object Shader | Apple Silicon |

## 検証

- [ ] 機能検出の正確性
- [ ] DispatchMesh動作
- [ ] メッシュレット構造体のGPUレイアウト一致
- [ ] 増幅シェーダーとの連携
