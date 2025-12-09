//----------------------------------------------------------------------------
//! @file   postprocess_ps.hlsl
//! @brief  ポストプロセス用ピクセルシェーダー（テスト用）
//!
//! @details
//! このシェーダーは以下をテストします：
//! - ポストプロセスパイプラインの基本構造
//! - 画像の明るさ・コントラスト・彩度調整
//! - テクセルサイズを使用した座標計算
//!
//! 使用例：fullscreen_vs.hlslと組み合わせてポストプロセスエフェクトを適用
//----------------------------------------------------------------------------

//! ソーステクスチャ（シーンのレンダリング結果）
//! @note t0レジスタにバインドされる
Texture2D sourceTexture : register(t0);

//! ポイントサンプラー（ピクセル単位でサンプリング）
//! @note s0レジスタにバインドされる
SamplerState pointSampler : register(s0);

//! ポストプロセスパラメータ用定数バッファ
//! @note b0レジスタにバインドされる
cbuffer PostProcessParams : register(b0)
{
    float Brightness;    //!< 明るさ調整 (-1.0 ～ 1.0、0.0で変化なし)
    float Contrast;      //!< コントラスト (0.0 ～ 2.0、1.0で変化なし)
    float Saturation;    //!< 彩度 (0.0 ～ 2.0、1.0で変化なし、0.0でグレースケール)
    float _pad0;
    float2 TexelSize;    //!< 1ピクセルのUV空間でのサイズ (1/width, 1/height)
    float2 _pad1;
};

//! ピクセルシェーダー入力構造体
struct PSInput
{
    float4 position : SV_POSITION;  //!< スクリーン空間座標
    float2 texCoord : TEXCOORD0;    //!< テクスチャ座標
};

//! 彩度調整関数
//! @param [in] color 入力カラー（RGB）
//! @param [in] saturation 彩度係数（0.0でグレースケール、1.0で変化なし）
//! @return 彩度調整後のカラー
float3 AdjustSaturation(float3 color, float saturation)
{
    // 輝度計算（ITU-R BT.601係数）
    float grey = dot(color, float3(0.299, 0.587, 0.114));

    // グレースケールと元の色を補間
    return lerp(float3(grey, grey, grey), color, saturation);
}

//! ポストプロセスピクセルシェーダーメイン関数
//! @param [in] input ラスタライザからの入力
//! @return レンダーターゲットへの出力カラー
float4 PSMain(PSInput input) : SV_TARGET
{
    // ソーステクスチャからサンプリング
    float4 color = sourceTexture.Sample(pointSampler, input.texCoord);

    // 明るさ調整（加算）
    color.rgb += Brightness;

    // コントラスト調整（0.5を中心にスケーリング）
    color.rgb = (color.rgb - 0.5) * Contrast + 0.5;

    // 彩度調整
    color.rgb = AdjustSaturation(color.rgb, Saturation);

    // 結果を0-1にクランプ（HDRの場合は不要）
    color.rgb = saturate(color.rgb);

    return color;
}
