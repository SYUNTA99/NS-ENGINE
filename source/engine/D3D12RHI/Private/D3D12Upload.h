/// @file D3D12Upload.h
/// @brief D3D12 アップロードヘルパー
#pragma once

#include "D3D12RHIPrivate.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12UploadHelper — 一時アップロードバッファ管理
    //=========================================================================

    /// CPU→GPU転送用の一時バッファを作成・管理する
    /// 作成された一時バッファは遅延削除キューに登録される
    struct D3D12UploadHelper
    {
        /// 一時アップロードバッファを作成してデータをコピー
        /// @param device D3D12デバイス
        /// @param data ソースデータ
        /// @param size データサイズ
        /// @param alignment アライメント（0=デフォルト）
        /// @return 作成されたID3D12Resourceと一時バッファ（失敗時nullptr）
        static ComPtr<ID3D12Resource> CreateUploadBuffer(D3D12Device* device,
                                                         const void* data,
                                                         uint64 size,
                                                         uint64 alignment = 0);
    };

} // namespace NS::D3D12RHI
