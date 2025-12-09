//----------------------------------------------------------------------------
//! @file   gpu.h
//! @brief  GPUリソース統合ヘッダー
//! @note   std::shared_ptr による参照管理
//!
//! Usage:
//!   #include "dx11/gpu/gpu.h"
//!   BufferPtr vb = ...;
//!   TexturePtr tex = ...;
//!   ShaderPtr vs = ...;
//----------------------------------------------------------------------------
#pragma once

#include "gpu_resource.h"
#include "buffer.h"
#include "texture.h"
#include "shader.h"
#include "dx11/view/shader_resource_view.h"
#include "dx11/view/unordered_access_view.h"

// サイズ検証（コンパイル時）
static_assert(sizeof(BufferDesc) == 24,   "BufferDesc must be 24 bytes");
static_assert(sizeof(TextureDesc) == 48,  "TextureDesc must be 48 bytes");
