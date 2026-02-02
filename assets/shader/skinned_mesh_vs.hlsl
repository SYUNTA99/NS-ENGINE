//----------------------------------------------------------------------------
// skinned_mesh_vs.hlsl
// Skinned Mesh vertex shader with bone animation support
//----------------------------------------------------------------------------

//! 最大ボーン数（定数バッファサイズ制限: 64KB = 約1000行列）
#define MAX_BONES 256

//============================================================================
// 定数バッファ
//============================================================================

// b0: フレーム定数
cbuffer PerFrame : register(b0)
{
    matrix viewProjection;
    float4 cameraPosition;
};

// b1: オブジェクト定数
cbuffer PerObject : register(b1)
{
    matrix world;
    matrix worldInvTranspose;
};

// b2: ボーン行列（スキニング行列 = 逆バインド × グローバル変換）
cbuffer BoneMatrices : register(b2)
{
    matrix bones[MAX_BONES];
};

//============================================================================
// 入出力構造体
//============================================================================

struct VSInput
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float4 tangent      : TANGENT;
    float2 texCoord     : TEXCOORD0;
    float4 color        : COLOR0;
    uint4  boneIndices  : BLENDINDICES;  // 4つのボーンインデックス
    float4 boneWeights  : BLENDWEIGHT;   // 4つのボーンウェイト
};

struct VSOutput
{
    float4 position     : SV_POSITION;
    float3 worldPos     : TEXCOORD0;
    float3 worldNormal  : TEXCOORD1;
    float3 worldTangent : TEXCOORD2;
    float3 worldBinorm  : TEXCOORD3;
    float2 texCoord     : TEXCOORD4;
    float4 color        : COLOR0;
};

//============================================================================
// スキニング計算
//============================================================================

// 4ボーンスキニングで位置を変換
float3 SkinPosition(float3 position, uint4 boneIndices, float4 boneWeights)
{
    float3 result = float3(0, 0, 0);

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float weight = boneWeights[i];
        if (weight > 0.0)
        {
            uint boneIndex = boneIndices[i];
            result += weight * mul(float4(position, 1.0), bones[boneIndex]).xyz;
        }
    }

    return result;
}

// 4ボーンスキニングで法線/タンジェントを変換（回転のみ）
float3 SkinNormal(float3 normal, uint4 boneIndices, float4 boneWeights)
{
    float3 result = float3(0, 0, 0);

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float weight = boneWeights[i];
        if (weight > 0.0)
        {
            uint boneIndex = boneIndices[i];
            // 法線は回転部分（3x3）のみで変換
            result += weight * mul(normal, (float3x3)bones[boneIndex]);
        }
    }

    return result;
}

//============================================================================
// メイン
//============================================================================

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // デバッグ: スキニング無効化テスト（0=スキニング有効、1=無効）
    #define DEBUG_DISABLE_SKINNING 0

    #if DEBUG_DISABLE_SKINNING
    // スキニング無効 - 元の頂点位置を使用
    float3 skinnedPosition = input.position;
    float3 skinnedNormal = input.normal;
    float3 skinnedTangent = input.tangent.xyz;
    #else
    // スキニング適用
    float3 skinnedPosition = SkinPosition(input.position, input.boneIndices, input.boneWeights);
    float3 skinnedNormal = SkinNormal(input.normal, input.boneIndices, input.boneWeights);
    float3 skinnedTangent = SkinNormal(input.tangent.xyz, input.boneIndices, input.boneWeights);
    #endif

    // ワールド座標
    float4 worldPosition = mul(float4(skinnedPosition, 1.0), world);
    output.worldPos = worldPosition.xyz;

    // クリップ座標
    output.position = mul(worldPosition, viewProjection);

    // 法線をワールド空間に変換（逆転置行列を使用）
    output.worldNormal = normalize(mul(float4(skinnedNormal, 0.0), worldInvTranspose).xyz);

    // タンジェントをワールド空間に変換
    output.worldTangent = normalize(mul(float4(skinnedTangent, 0.0), world).xyz);

    // バイノーマルを計算（タンジェントのw成分でハンドネス）
    output.worldBinorm = cross(output.worldNormal, output.worldTangent) * input.tangent.w;

    // テクスチャ座標（Unity FBXはV座標が反転しているので修正）
    output.texCoord = float2(input.texCoord.x, 1.0 - input.texCoord.y);

    // 頂点カラー
    output.color = input.color;

    return output;
}
