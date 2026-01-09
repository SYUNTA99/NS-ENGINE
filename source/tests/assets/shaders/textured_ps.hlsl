//----------------------------------------------------------------------------
//! @file   textured_ps.hlsl
//! @brief  テクスチャ付きライティング用ピクセルシェーダー（テスト用）
//!
//! @details
//! このシェーダーは以下をテストします：
//! - Texture2Dリソースの使用
//! - SamplerStateの使用
//! - ピクセルシェーダー用定数バッファ
//! - ディフューズライティング（ランバート反射）
//!
//! 使用例：textured_vs.hlslと組み合わせてライティング付きテクスチャメッシュを描画
//----------------------------------------------------------------------------

//! ディフューズテクスチャ
//! @note t0レジスタにバインドされる
Texture2D diffuseTexture : register(t0);

//! リニアサンプラー
//! @note s0レジスタにバインドされる
SamplerState linearSampler : register(s0);

//! ライティング用定数バッファ
//! @note b0レジスタにバインドされる
cbuffer LightBuffer : register(b0)
{
    float3 LightDirection;  //!< ライト方向（正規化済み、ワールド空間）
    float _pad0;            //!< 16バイトアライメント用パディング
    float3 LightColor;      //!< ライトカラー（RGB、HDR可）
    float _pad1;
    float3 AmbientColor;    //!< 環境光カラー（RGB）
    float _pad2;
};

//! ピクセルシェーダー入力構造体
struct PSInput
{
    float4 position : SV_POSITION;  //!< スクリーン空間座標
    float3 normal : NORMAL;         //!< 補間されたワールド空間法線
    float2 texCoord : TEXCOORD0;    //!< テクスチャ座標
    float3 worldPos : TEXCOORD1;    //!< ワールド空間位置（未使用だが拡張用）
};

//! ピクセルシェーダーメイン関数
//! @param [in] input ラスタライザからの入力
//! @return レンダーターゲットへの出力カラー
float4 PSMain(PSInput input) : SV_TARGET
{
    // テクスチャサンプリング
    float4 texColor = diffuseTexture.Sample(linearSampler, input.texCoord);

    // ランバート反射（ディフューズライティング）
    float3 N = normalize(input.normal);  // 補間で非正規化されるため再正規化
    float3 L = normalize(-LightDirection);  // ライト方向を反転
    float NdotL = max(dot(N, L), 0.0);  // 裏面は0にクランプ

    // ライティング計算
    float3 diffuse = LightColor * NdotL;
    float3 ambient = AmbientColor;

    // 最終カラー = テクスチャカラー × (ディフューズ + アンビエント)
    float3 finalColor = texColor.rgb * (diffuse + ambient);

    return float4(finalColor, texColor.a);
}
