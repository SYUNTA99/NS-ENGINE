/// @file IRHIMeshShader.h
/// @brief メッシュシェーダー・増幅シェーダーインターフェース
/// @details IRHIShaderを継承するメッシュシェーダーと増幅シェーダーのインターフェースを提供。
/// @see 22-01-mesh-shader.md
#pragma once

#include "IRHIShader.h"
#include "RHIMacros.h"

namespace NS::RHI
{

    //=========================================================================
    // IRHIMeshShader (22-01)
    //=========================================================================

    /// メッシュシェーダー
    /// 頂点処理とプリミティブ生成を統合
    class RHI_API IRHIMeshShader : public IRHIShader
    {
    public:
        virtual ~IRHIMeshShader() = default;

        /// シェーダーステージ取得
        EShaderFrequency GetFrequency() const override { return EShaderFrequency::Mesh; }

        //=====================================================================
        // メッシュシェーダー固有情報
        //=====================================================================

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

    //=========================================================================
    // IRHIAmplificationShader (22-01)
    //=========================================================================

    /// 増幅（タスク/アンプリフィケーション）シェーダー
    /// メッシュシェーダーの起動を制御
    class RHI_API IRHIAmplificationShader : public IRHIShader
    {
    public:
        virtual ~IRHIAmplificationShader() = default;

        /// シェーダーステージ取得
        EShaderFrequency GetFrequency() const override { return EShaderFrequency::Amplification; }

        //=====================================================================
        // 増幅シェーダー固有情報
        //=====================================================================

        /// ペイロードサイズ取得（メッシュシェーダーへ渡すデータサイズ）
        virtual uint32 GetPayloadSize() const = 0;

        /// スレッドグループサイズ取得
        virtual void GetThreadGroupSize(uint32& x, uint32& y, uint32& z) const = 0;
    };

    using RHIAmplificationShaderRef = TRefCountPtr<IRHIAmplificationShader>;

} // namespace NS::RHI
