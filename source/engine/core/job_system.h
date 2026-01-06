//----------------------------------------------------------------------------
//! @file   job_system.h
//! @brief  マルチスレッドジョブシステム
//----------------------------------------------------------------------------
#pragma once

#include "common/utility/non_copyable.h"
#include <functional>
#include <atomic>
#include <memory>
#include <cstdint>
#include <cassert>

//! @brief ジョブ優先度
enum class JobPriority : uint8_t {
    High = 0,    //!< 高優先度（フレーム内で必ず完了）
    Normal = 1,  //!< 通常
    Low = 2,     //!< 低優先度（バックグラウンド処理）
    Count = 3
};

//============================================================================
//! @brief ジョブカウンター（依存関係管理用）
//!
//! 複数のジョブが完了するまで待機するために使用。
//! カウンターが0になると待機中のスレッドに通知。
//!
//! @note 使用例:
//! @code
//!   auto counter = std::make_shared<JobCounter>(3);
//!   JobSystem::Get().Submit([]{ Work1(); }, counter);
//!   JobSystem::Get().Submit([]{ Work2(); }, counter);
//!   JobSystem::Get().Submit([]{ Work3(); }, counter);
//!   counter->Wait();  // 3つ全て完了まで待機
//! @endcode
//============================================================================
class JobCounter
{
public:
    JobCounter();

    //! @brief 初期カウント指定コンストラクタ
    //! @param initialCount 初期カウント値
    explicit JobCounter(uint32_t initialCount);

    ~JobCounter();

    // コピー禁止（mutex/condition_variableはコピー不可）
    JobCounter(const JobCounter&) = delete;
    JobCounter& operator=(const JobCounter&) = delete;

    //! @brief カウントをデクリメント（ジョブ完了時に呼び出し）
    void Decrement() noexcept;

    //! @brief カウントが0になるまで待機
    void Wait() const noexcept;

    //! @brief カウントが0かどうか
    [[nodiscard]] bool IsComplete() const noexcept;

    //! @brief 現在のカウント
    [[nodiscard]] uint32_t GetCount() const noexcept;

    //! @brief カウントをリセット
    //! @param count 新しいカウント値
    void Reset(uint32_t count) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

using JobCounterPtr = std::shared_ptr<JobCounter>;

//! @brief ジョブ関数型
using JobFunction = std::function<void()>;

//============================================================================
//! @brief ジョブシステム（シングルトン）
//!
//! ワーカースレッドプールを管理し、ジョブを並列実行する。
//!
//! @note 使用例:
//! @code
//!   // 単発ジョブ（Fire-and-forget）
//!   JobSystem::Get().Submit([]{ DoWork(); });
//!
//!   // 待機可能ジョブ
//!   auto counter = JobSystem::Get().SubmitAndGetCounter([]{ DoWork(); });
//!   counter->Wait();
//!
//!   // 複数ジョブを待機
//!   auto counter = std::make_shared<JobCounter>(3);
//!   JobSystem::Get().Submit([]{ Work1(); }, counter);
//!   JobSystem::Get().Submit([]{ Work2(); }, counter);
//!   JobSystem::Get().Submit([]{ Work3(); }, counter);
//!   counter->Wait();  // 3つ全て完了まで待機
//!
//!   // 並列forループ
//!   JobSystem::Get().ParallelFor(0, 1000, [](uint32_t i) {
//!       ProcessItem(i);
//!   });
//! @endcode
//============================================================================
class JobSystem final : private NonCopyableNonMovable
{
public:
    //! @brief シングルトン取得
    static JobSystem& Get() noexcept {
        assert(instance_ && "JobSystem::Create() must be called first");
        return *instance_;
    }

    //! @brief インスタンス生成
    //! @param numWorkers ワーカースレッド数（0でCPUコア数-1）
    static void Create(uint32_t numWorkers = 0);

    //! @brief インスタンス破棄
    static void Destroy();

    //! @brief インスタンスが存在するか
    [[nodiscard]] static bool IsCreated() noexcept { return instance_ != nullptr; }

    //------------------------------------------------------------------------
    //! @name ジョブ投入
    //------------------------------------------------------------------------
    //!@{

    //! @brief ジョブを投入（Fire-and-forget）
    //! @param job 実行する関数
    //! @param priority 優先度
    void Submit(JobFunction job, JobPriority priority = JobPriority::Normal);

    //! @brief ジョブを投入（カウンター付き）
    //! @param job 実行する関数
    //! @param counter 完了時にデクリメントするカウンター
    //! @param priority 優先度
    void Submit(JobFunction job, JobCounterPtr counter, JobPriority priority = JobPriority::Normal);

    //! @brief ジョブを投入し、完了カウンターを取得
    //! @param job 実行する関数
    //! @param priority 優先度
    //! @return 完了待機用カウンター
    [[nodiscard]] JobCounterPtr SubmitAndGetCounter(JobFunction job, JobPriority priority = JobPriority::Normal);

    //!@}

    //------------------------------------------------------------------------
    //! @name 並列ループ
    //------------------------------------------------------------------------
    //!@{

    //! @brief 並列forループ
    //! @param begin 開始インデックス
    //! @param end 終了インデックス（含まない）
    //! @param func 各インデックスで実行する関数
    //! @param granularity 1ジョブあたりの処理数（0で自動）
    void ParallelFor(uint32_t begin, uint32_t end,
                     const std::function<void(uint32_t)>& func,
                     uint32_t granularity = 0);

    //! @brief 並列forループ（範囲版）
    //! @param begin 開始インデックス
    //! @param end 終了インデックス（含まない）
    //! @param func 範囲を受け取る関数 (begin, end)
    //! @param granularity 1ジョブあたりの処理数（0で自動）
    void ParallelForRange(uint32_t begin, uint32_t end,
                          const std::function<void(uint32_t, uint32_t)>& func,
                          uint32_t granularity = 0);

    //!@}

    //------------------------------------------------------------------------
    //! @name 状態取得
    //------------------------------------------------------------------------
    //!@{

    //! @brief ワーカースレッド数を取得
    [[nodiscard]] uint32_t GetWorkerCount() const noexcept;

    //! @brief 現在のスレッドがワーカースレッドかどうか
    [[nodiscard]] bool IsWorkerThread() const noexcept;

    //! @brief 保留中のジョブ数を取得
    [[nodiscard]] uint32_t GetPendingJobCount() const noexcept;

    //!@}

    //! @brief デストラクタ
    ~JobSystem();

private:
    JobSystem() = default;

    void Initialize(uint32_t numWorkers);
    void Shutdown();

    class Impl;
    std::unique_ptr<Impl> impl_;

    static inline std::unique_ptr<JobSystem> instance_ = nullptr;
};
