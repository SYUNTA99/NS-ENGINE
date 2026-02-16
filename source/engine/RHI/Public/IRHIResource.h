/// @file IRHIResource.h
/// @brief RHIリソース基底クラス
/// @details 全RHIリソースの共通インターフェース。参照カウント、デバッグ名、遅延削除を提供。
/// @see 01-12-resource-base.md
#pragma once

#include "Common/Utility/Macros.h"
#include "Common/Utility/Types.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHITypes.h"
#include <atomic>
#include <cstring>
#include <mutex>
#include <string>

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIResourceType
    //=========================================================================

    /// RHIリソースタイプ
    enum class ERHIResourceType : uint8
    {
        Unknown = 0,

        // GPU リソース
        Buffer,
        Texture,

        // ビュー
        ShaderResourceView,
        UnorderedAccessView,
        RenderTargetView,
        DepthStencilView,
        ConstantBufferView,

        // サンプラー
        Sampler,

        // シェーダー・パイプライン
        Shader,
        GraphicsPipelineState,
        ComputePipelineState,
        RootSignature,

        // コマンド
        CommandList,
        CommandAllocator,

        // 同期
        Fence,
        SyncPoint,

        // ディスクリプタ
        DescriptorHeap,

        // クエリ
        QueryHeap,

        // スワップチェーン
        SwapChain,

        // レイトレーシング
        AccelerationStructure,
        RayTracingPSO,
        ShaderBindingTable,

        // メモリ
        Heap,

        // その他
        InputLayout,
        ShaderLibrary,
        ResourceCollection,

        Count
    };

    /// 前方宣言
    class RHIDeferredDeleteQueue;

    //=========================================================================
    // IRHIResource
    //=========================================================================

    /// RHIリソース基底クラス
    /// 全てのRHIリソースはこのクラスを継承する
    class RHI_API IRHIResource
    {
    public:
        virtual ~IRHIResource();

        NS_DISALLOW_COPY(IRHIResource);

        //=====================================================================
        // 参照カウント
        //=====================================================================

        /// 参照カウント増加（スレッドセーフ）
        /// @return 増加後の参照カウント
        uint32 AddRef() const noexcept;

        /// 参照カウント減少（スレッドセーフ）
        /// @return 減少後の参照カウント（0なら削除された）
        uint32 Release() const noexcept;

        /// 現在の参照カウント取得
        uint32 GetRefCount() const noexcept;

        //=====================================================================
        // リソース識別
        //=====================================================================

        /// リソースタイプ取得
        ERHIResourceType GetResourceType() const noexcept { return m_resourceType; }

        /// リソースID取得
        ResourceId GetResourceId() const noexcept { return m_resourceId; }

        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグ名設定（スレッドセーフ）
        /// @param name UTF-8文字列
        virtual void SetDebugName(const char* name);

        /// デバッグ名取得（スレッドセーフ）
        /// @param outBuffer 出力バッファ
        /// @param bufferSize バッファサイズ
        /// @return 書き込まれた文字数
        size_t GetDebugName(char* outBuffer, size_t bufferSize) const;

        /// デバッグ名があるか
        bool HasDebugName() const;

        //=====================================================================
        // 遅延削除
        //=====================================================================

        /// 遅延削除としてマーク
        void MarkForDeferredDelete() const noexcept;

        /// 遅延削除待ちか
        bool IsPendingDelete() const noexcept;

        //=====================================================================
        // マーカー（型判定）
        //=====================================================================

        /// バッファか
        virtual bool IsBuffer() const { return false; }

        /// テクスチャか
        virtual bool IsTexture() const { return false; }

        /// ビューか
        virtual bool IsView() const { return false; }

        //=====================================================================
        // GPU常駐状態（Residency対応用）
        //=====================================================================

        /// GPUメモリに常駐しているか
        virtual bool IsResident() const { return true; }

        /// 常駐優先度設定
        virtual void SetResidencyPriority(uint32 priority) { (void)priority; }

    protected:
        /// 派生クラスのみ構築可能
        explicit IRHIResource(ERHIResourceType type);

        /// 参照カウントが0になった時の処理
        /// デフォルトでは即座に削除。遅延削除が必要な場合はオーバーライド。
        virtual void OnZeroRefCount() const;

    private:
        mutable std::atomic<uint32> m_refCount{1};
        ResourceId m_resourceId;
        ERHIResourceType m_resourceType;
        mutable std::atomic<bool> m_pendingDelete{false};

        std::string m_debugName;
        mutable std::mutex m_debugNameMutex;

        friend class RHIDeferredDeleteQueue;

        /// 遅延削除キューから呼び出し
        void ExecuteDeferredDelete() const;
    };

    //=========================================================================
    // RHIResourceLocation
    //=========================================================================

    /// リソースメモリロケーション
    /// GPUリソースの実際のメモリ位置を示す
    struct RHI_API RHIResourceLocation
    {
        /// 基盤リソースへの参照
        IRHIResource* resource = nullptr;

        /// リソース内オフセット（サブアロケーション時）
        uint64 offset = 0;

        /// 割り当てサイズ
        uint64 size = 0;

        /// GPU仮想アドレス
        uint64 gpuVirtualAddress = 0;

        /// 有効か
        bool IsValid() const { return resource != nullptr; }

        /// GPU仮想アドレス取得（オフセット適用済み）
        uint64 GetGPUVirtualAddress() const { return gpuVirtualAddress + offset; }
    };

    //=========================================================================
    // リソースタイプ別RefCountPtrエイリアス
    //=========================================================================

    using RHIResourceRef = TRefCountPtr<IRHIResource>;

}} // namespace NS::RHI
