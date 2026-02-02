//----------------------------------------------------------------------------
//! @file   memory_system.cpp
//! @brief  メモリシステム実装
//----------------------------------------------------------------------------
#include "memory_system.h"
#include "common/logging/logging.h"
#include <cassert>
#include <cstdio>

namespace Memory {

//----------------------------------------------------------------------------
void MemorySystem::Initialize() {
    if (initialized_) {
        LOG_WARN("[MemorySystem] Already initialized");
        return;
    }

    LOG_INFO("[MemorySystem] Initializing...");

    // フレームアロケータを作成（1MB）
    frameAllocator_ = std::make_unique<LinearAllocator>(
        kFrameAllocatorCapacity, &defaultAllocator_);

    // Chunk用プールを作成
    chunkPool_ = std::make_unique<PoolAllocator<kChunkBlockSize>>(
        &defaultAllocator_);

    initialized_ = true;

    char buf[128];
    snprintf(buf, sizeof(buf), "[MemorySystem] Frame allocator: %zu KB",
             kFrameAllocatorCapacity / 1024);
    LOG_INFO(buf);

    snprintf(buf, sizeof(buf), "[MemorySystem] Chunk pool block size: %zu KB",
             kChunkBlockSize / 1024);
    LOG_INFO(buf);

    LOG_INFO("[MemorySystem] Initialized successfully");
}

//----------------------------------------------------------------------------
void MemorySystem::Shutdown() {
    if (!initialized_) {
        return;
    }

    LOG_INFO("[MemorySystem] Shutting down...");

    // 終了時の統計を出力
    DumpStats();

    // メモリリーク警告
    const auto defaultStats = defaultAllocator_.GetStats();

    // Chunkプールの統計
    if (chunkPool_) {
        const auto poolStats = chunkPool_->GetStats();
        if (poolStats.currentUsed > 0) {
            LOG_WARN("[MemorySystem] Chunk pool has unreleased memory!");

            char buf[256];
            snprintf(buf, sizeof(buf), "  Used: %zu bytes (%zu blocks)",
                     poolStats.currentUsed,
                     poolStats.currentUsed / kChunkBlockSize);
            LOG_WARN(buf);
        }
    }

    // デフォルトアロケータの統計（プール解放後に確認）
    // 解放順序: プール → フレームアロケータ
    chunkPool_.reset();
    frameAllocator_.reset();

    // 最終チェック
    const auto finalStats = defaultAllocator_.GetStats();
    if (finalStats.currentUsed > 0) {
        LOG_WARN("[MemorySystem] Potential memory leak detected!");

        char buf[256];
        snprintf(buf, sizeof(buf), "  Default allocator: %zu bytes still in use",
                 finalStats.currentUsed);
        LOG_WARN(buf);

        snprintf(buf, sizeof(buf), "  Allocations: %zu, Deallocations: %zu",
                 finalStats.allocationCount, finalStats.deallocationCount);
        LOG_WARN(buf);
    }

    initialized_ = false;
    LOG_INFO("[MemorySystem] Shutdown complete");
}

//----------------------------------------------------------------------------
void MemorySystem::BeginFrame() {
    if (frameAllocator_) {
        frameAllocator_->Reset();
    }
}

//----------------------------------------------------------------------------
void MemorySystem::EndFrame() {
    // 統計更新やデバッグチェックをここで行う
}

//----------------------------------------------------------------------------
void MemorySystem::DumpStats() const {
    LOG_INFO("=== Memory System Stats ===");

    char buf[256];

    // デフォルトアロケータ
    const auto defaultStats = defaultAllocator_.GetStats();
    snprintf(buf, sizeof(buf), "[%s]", defaultAllocator_.GetName());
    LOG_INFO(buf);

    snprintf(buf, sizeof(buf), "  Current:      %zu bytes", defaultStats.currentUsed);
    LOG_INFO(buf);

    snprintf(buf, sizeof(buf), "  Peak:         %zu bytes", defaultStats.peakUsed);
    LOG_INFO(buf);

    snprintf(buf, sizeof(buf), "  Total:        %zu bytes", defaultStats.totalAllocated);
    LOG_INFO(buf);

    snprintf(buf, sizeof(buf), "  Allocations:  %zu", defaultStats.allocationCount);
    LOG_INFO(buf);

    snprintf(buf, sizeof(buf), "  Deallocations:%zu", defaultStats.deallocationCount);
    LOG_INFO(buf);

    // フレームアロケータ
    if (frameAllocator_) {
        snprintf(buf, sizeof(buf), "[%s]", frameAllocator_->GetName());
        LOG_INFO(buf);

        snprintf(buf, sizeof(buf), "  Used:         %zu / %zu bytes (%.1f%%)",
                 frameAllocator_->GetUsed(),
                 frameAllocator_->GetCapacity(),
                 frameAllocator_->GetUsageRatio() * 100.0f);
        LOG_INFO(buf);
    }

    // Chunkプール
    if (chunkPool_) {
        const auto poolStats = chunkPool_->GetStats();
        snprintf(buf, sizeof(buf), "[%s (Chunk)]", chunkPool_->GetName());
        LOG_INFO(buf);

        snprintf(buf, sizeof(buf), "  Current:      %zu bytes (%zu blocks)",
                 poolStats.currentUsed,
                 chunkPool_->GetUsedBlockCount());
        LOG_INFO(buf);

        snprintf(buf, sizeof(buf), "  Chunks:       %zu (%zu blocks total)",
                 chunkPool_->GetChunkCount(),
                 chunkPool_->GetTotalBlockCount());
        LOG_INFO(buf);
    }

    LOG_INFO("=== Total ===");

    snprintf(buf, sizeof(buf), "  Current:      %zu bytes", GetTotalAllocated());
    LOG_INFO(buf);

    snprintf(buf, sizeof(buf), "  Peak:         %zu bytes", GetPeakAllocated());
    LOG_INFO(buf);

    LOG_INFO("===========================");
}

//----------------------------------------------------------------------------
size_t MemorySystem::GetTotalAllocated() const noexcept {
    size_t total = 0;
    total += defaultAllocator_.GetStats().currentUsed;

    if (frameAllocator_) {
        total += frameAllocator_->GetUsed();
    }

    if (chunkPool_) {
        total += chunkPool_->GetStats().currentUsed;
    }

    return total;
}

//----------------------------------------------------------------------------
size_t MemorySystem::GetPeakAllocated() const noexcept {
    size_t peak = 0;
    peak += defaultAllocator_.GetStats().peakUsed;

    if (frameAllocator_) {
        // LinearAllocatorはピークを別途追跡していないので、容量を使用
        peak += frameAllocator_->GetCapacity();
    }

    if (chunkPool_) {
        peak += chunkPool_->GetStats().peakUsed;
    }

    return peak;
}

} // namespace Memory
