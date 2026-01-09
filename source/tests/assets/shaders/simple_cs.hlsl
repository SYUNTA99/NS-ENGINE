//----------------------------------------------------------------------------
//! @file   simple_cs.hlsl
//! @brief  基本的なコンピュートシェーダー（テスト用）
//!
//! @details
//! このシェーダーは以下をテストします：
//! - コンピュートシェーダーの基本的なコンパイル
//! - RWBuffer（読み書き可能バッファ）へのアクセス
//! - SV_DispatchThreadIDセマンティクスの使用
//! - numthreads属性の指定
//!
//! 使用例：GPUでの並列データ処理（各要素を2倍にする）
//! Dispatch(N/64, 1, 1) で N 要素を処理
//----------------------------------------------------------------------------

//! 出力バッファ（UAV: Unordered Access View）
//! @note u0レジスタにバインドされる
RWBuffer<float> output : register(u0);

//! コンピュートシェーダーメイン関数
//! @param [in] DTid ディスパッチ全体でのスレッドID
//!
//! @note numthreads(64, 1, 1) は1つのスレッドグループあたり64スレッドを指定
//!       一般的なGPUでは32または64の倍数が効率的
[numthreads(64, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // スレッドIDに対応する要素を2倍にして出力
    // 例：DTid.x = 5 の場合、output[5] = 10.0
    output[DTid.x] = float(DTid.x) * 2.0;
}
