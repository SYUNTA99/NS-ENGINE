//----------------------------------------------------------------------------
//! @file   skinning_vs.hlsl
//! @brief  スキニングメッシュ用頂点シェーダー（テスト用）
//!
//! @details
//! このシェーダーは以下をテストします：
//! - 大きな定数バッファ配列（ボーン行列256個）
//! - BLENDINDICES/BLENDWEIGHTセマンティクス
//! - 頂点ブレンディング（4ボーンスキニング）
//! - ループのアンロール最適化
//!
//! 使用例：スケルタルアニメーションを使用したキャラクターメッシュ描画
//!
//! @note スキニングの流れ：
//!       1. 各頂点は最大4つのボーンの影響を受ける
//!       2. 各ボーンの変換行列を重み付けして合成
//!       3. 合成した行列で頂点位置と法線を変換
//----------------------------------------------------------------------------

//! 最大ボーン数
//! @note GPUの定数バッファサイズ制限に注意（64KB = 約1000行列）
#define MAX_BONES 256

//! 変換行列用定数バッファ
//! @note b0レジスタにバインドされる
cbuffer TransformBuffer : register(b0)
{
    matrix World;       //!< ワールド変換行列
    matrix View;        //!< ビュー変換行列
    matrix Projection;  //!< プロジェクション変換行列
};

//! ボーン行列用定数バッファ
//! @note b1レジスタにバインドされる
//! @note サイズ: 256 × 64バイト = 16KB
cbuffer BoneBuffer : register(b1)
{
    matrix BoneMatrices[MAX_BONES];  //!< ボーン変換行列配列（バインドポーズからの相対変換）
};

//! 頂点シェーダー入力構造体
struct VSInput
{
    float3 position : POSITION;       //!< 頂点位置（バインドポーズでのローカル座標）
    float3 normal : NORMAL;           //!< 法線ベクトル（バインドポーズでのローカル座標）
    float2 texCoord : TEXCOORD0;      //!< テクスチャ座標
    uint4 boneIndices : BLENDINDICES; //!< 影響を受けるボーンのインデックス（最大4つ）
    float4 boneWeights : BLENDWEIGHT; //!< 各ボーンの影響度（合計1.0になるよう正規化済み）
};

//! 頂点シェーダー出力構造体
struct VSOutput
{
    float4 position : SV_POSITION;  //!< クリップ空間座標
    float3 normal : NORMAL;         //!< ワールド空間法線
    float2 texCoord : TEXCOORD0;    //!< テクスチャ座標
};

//! スキニング頂点シェーダーメイン関数
//! @param [in] input 頂点入力データ（ボーンウェイト含む）
//! @return ピクセルシェーダーへの出力
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // スキニング後の位置と法線を初期化
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);

    // 4ボーンブレンディング
    // [unroll]属性でループをコンパイル時に展開し、GPUでの分岐を回避
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float weight = input.boneWeights[i];

        // ウェイトが0より大きい場合のみ処理
        // 注：多くのGPUでは条件分岐より常に計算した方が速い
        if (weight > 0.0)
        {
            uint boneIndex = input.boneIndices[i];
            matrix boneMatrix = BoneMatrices[boneIndex];

            // 位置のブレンド（行列×位置×ウェイト）
            skinnedPos += weight * mul(float4(input.position, 1.0), boneMatrix);

            // 法線のブレンド（回転部分のみ使用）
            skinnedNormal += weight * mul(input.normal, (float3x3)boneMatrix);
        }
    }

    // ワールド変換
    float4 worldPos = mul(skinnedPos, World);

    // ビュー・プロジェクション変換
    output.position = mul(mul(worldPos, View), Projection);

    // 法線をワールド空間に変換して正規化
    output.normal = normalize(mul(skinnedNormal, (float3x3)World));

    // テクスチャ座標はそのまま
    output.texCoord = input.texCoord;

    return output;
}
