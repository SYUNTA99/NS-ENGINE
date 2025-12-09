//----------------------------------------------------------------------------
//! @file   simple_vs.hlsl
//! @brief  基本的な頂点シェーダー（テスト用）
//!
//! @details
//! このシェーダーは以下をテストします：
//! - 頂点シェーダーの基本的なコンパイル
//! - POSITION/COLORセマンティクスの入力
//! - SV_POSITION/COLORセマンティクスの出力
//! - 頂点属性のパススルー（変換なし）
//!
//! 使用例：単色または頂点カラーのメッシュ描画
//----------------------------------------------------------------------------

//! 頂点シェーダー入力構造体
struct VSInput
{
    float3 position : POSITION;  //!< 頂点位置（ローカル座標）
    float4 color : COLOR;        //!< 頂点カラー（RGBA）
};

//! 頂点シェーダー出力構造体
struct VSOutput
{
    float4 position : SV_POSITION;  //!< クリップ空間座標
    float4 color : COLOR;           //!< 補間される頂点カラー
};

//! 頂点シェーダーメイン関数
//! @param [in] input 頂点入力データ
//! @return ピクセルシェーダーへの出力
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // 位置をそのままクリップ空間へ（W=1.0）
    // 注：実際の使用時はワールド・ビュー・プロジェクション変換が必要
    output.position = float4(input.position, 1.0);

    // カラーをそのまま出力（ラスタライザで補間される）
    output.color = input.color;

    return output;
}
