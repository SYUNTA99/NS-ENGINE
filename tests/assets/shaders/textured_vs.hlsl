//----------------------------------------------------------------------------
//! @file   textured_vs.hlsl
//! @brief  テクスチャ付きメッシュ用頂点シェーダー（テスト用）
//!
//! @details
//! このシェーダーは以下をテストします：
//! - 定数バッファ（cbuffer）の使用
//! - ワールド・ビュー・プロジェクション変換
//! - 法線ベクトルの変換
//! - テクスチャ座標のパススルー
//!
//! 使用例：ライティングとテクスチャを使用した3Dメッシュ描画
//----------------------------------------------------------------------------

//! 変換行列用定数バッファ
//! @note b0レジスタにバインドされる
cbuffer TransformBuffer : register(b0)
{
    matrix World;       //!< ワールド変換行列
    matrix View;        //!< ビュー変換行列
    matrix Projection;  //!< プロジェクション変換行列
};

//! 頂点シェーダー入力構造体
struct VSInput
{
    float3 position : POSITION;   //!< 頂点位置（ローカル座標）
    float3 normal : NORMAL;       //!< 法線ベクトル（ローカル座標）
    float2 texCoord : TEXCOORD0;  //!< テクスチャ座標（UV）
};

//! 頂点シェーダー出力構造体
struct VSOutput
{
    float4 position : SV_POSITION;  //!< クリップ空間座標
    float3 normal : NORMAL;         //!< ワールド空間法線
    float2 texCoord : TEXCOORD0;    //!< テクスチャ座標
    float3 worldPos : TEXCOORD1;    //!< ワールド空間位置（ライティング用）
};

//! 頂点シェーダーメイン関数
//! @param [in] input 頂点入力データ
//! @return ピクセルシェーダーへの出力
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // ワールド変換
    float4 worldPos = mul(float4(input.position, 1.0), World);
    output.worldPos = worldPos.xyz;

    // ビュー・プロジェクション変換
    output.position = mul(mul(worldPos, View), Projection);

    // 法線をワールド空間に変換（スケーリングに対応するため3x3部分のみ使用）
    // 注：非均一スケールの場合は逆転置行列を使用すべき
    output.normal = normalize(mul(input.normal, (float3x3)World));

    // テクスチャ座標はそのまま出力
    output.texCoord = input.texCoord;

    return output;
}
