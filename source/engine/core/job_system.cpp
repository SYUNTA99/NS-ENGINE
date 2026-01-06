//----------------------------------------------------------------------------
//! @file   job_system.cpp
//! @brief  マルチスレッドジョブシステム 実装
//----------------------------------------------------------------------------

// Windows.hのmin/maxマクロを無効化（最初に定義）
#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
#endif

#include "job_system.h"
#include "common/logging/logging.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include <algorithm>

// マクロ干渉回避用
#undef max
#undef min

//----------------------------------------------------------------------------
// JobCounter 実装
//----------------------------------------------------------------------------

void JobCounter::Decrement() noexcept
{
    uint32_t prev = count_.fetch_sub(1, std::memory_order_acq_rel);
    // カウントが1から0になった場合、待機中のスレッドに通知
    // Note: 通知は waiting_ フラグで最適化（誰も待っていなければスキップ）
    if (prev == 1 && waiting_.load(std::memory_order_acquire)) {
        waiting_.store(false, std::memory_order_release);
    }
}

void JobCounter::Wait() const noexcept
{
    // 既に完了していれば即座にリターン
    if (count_.load(std::memory_order_acquire) == 0) {
        return;
    }

    // 待機中フラグを立てる
    waiting_.store(true, std::memory_order_release);

    // スピンウェイト + バックオフ
    uint32_t spinCount = 0;
    constexpr uint32_t kMaxSpinCount = 1000;

    while (count_.load(std::memory_order_acquire) != 0) {
        if (spinCount < kMaxSpinCount) {
            // スピンウェイト（CPU負荷軽減のためpause命令）
            #if defined(_MSC_VER)
                _mm_pause();
            #else
                __builtin_ia32_pause();
            #endif
            ++spinCount;
        } else {
            // スピン上限に達したらスリープ
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    waiting_.store(false, std::memory_order_release);
}

//----------------------------------------------------------------------------
// JobSystem::Impl
//----------------------------------------------------------------------------

class JobSystem::Impl
{
public:
    //! @brief ジョブデータ
    struct Job {
        JobFunction function;
        JobCounterPtr counter;
    };

    Impl() = default;
    ~Impl() { Shutdown(); }

    void Initialize(uint32_t numWorkers)
    {
        if (running_) return;

        // ワーカー数を決定（0なら論理コア数-1、最低1）
        if (numWorkers == 0) {
            numWorkers = std::max(1u, std::thread::hardware_concurrency() - 1);
        }

        running_ = true;
        workers_.reserve(numWorkers);

        for (uint32_t i = 0; i < numWorkers; ++i) {
            workers_.emplace_back(&Impl::WorkerThread, this, i);
        }

        LOG_INFO("[JobSystem] 初期化完了: ワーカースレッド数=" + std::to_string(numWorkers));
    }

    void Shutdown()
    {
        if (!running_) return;

        // シャットダウンを通知
        {
            std::unique_lock<std::mutex> lock(mutex_);
            running_ = false;
        }
        condition_.notify_all();

        // 全ワーカーの終了を待機
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers_.clear();

        // 残っているジョブをクリア
        for (int i = 0; i < static_cast<int>(JobPriority::Count); ++i) {
            queues_[i].clear();
        }

        LOG_INFO("[JobSystem] シャットダウン完了");
    }

    void Submit(JobFunction job, JobPriority priority)
    {
        Submit(std::move(job), nullptr, priority);
    }

    void Submit(JobFunction job, JobCounterPtr counter, JobPriority priority)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queues_[static_cast<int>(priority)].push_back({std::move(job), std::move(counter)});
            ++pendingJobs_;
        }
        condition_.notify_one();
    }

    JobCounterPtr SubmitAndGetCounter(JobFunction job, JobPriority priority)
    {
        auto counter = std::make_shared<JobCounter>(1);
        Submit(std::move(job), counter, priority);
        return counter;
    }

    void ParallelFor(uint32_t begin, uint32_t end,
                     const std::function<void(uint32_t)>& func,
                     uint32_t granularity)
    {
        if (begin >= end) return;

        uint32_t count = end - begin;

        // 粒度を自動計算（ワーカー数の2倍程度に分割）
        if (granularity == 0) {
            uint32_t numJobs = std::max(1u, static_cast<uint32_t>(workers_.size()) * 2);
            granularity = std::max(1u, count / numJobs);
        }

        // ジョブ数を計算
        uint32_t numJobs = (count + granularity - 1) / granularity;
        auto counter = std::make_shared<JobCounter>(numJobs);

        // 各範囲をジョブとして投入
        for (uint32_t i = 0; i < numJobs; ++i) {
            uint32_t jobBegin = begin + i * granularity;
            uint32_t jobEnd = std::min(jobBegin + granularity, end);

            Submit([func, jobBegin, jobEnd] {
                for (uint32_t j = jobBegin; j < jobEnd; ++j) {
                    func(j);
                }
            }, counter, JobPriority::Normal);
        }

        // 全ジョブの完了を待機
        counter->Wait();
    }

    void ParallelForRange(uint32_t begin, uint32_t end,
                          const std::function<void(uint32_t, uint32_t)>& func,
                          uint32_t granularity)
    {
        if (begin >= end) return;

        uint32_t count = end - begin;

        // 粒度を自動計算
        if (granularity == 0) {
            uint32_t numJobs = std::max(1u, static_cast<uint32_t>(workers_.size()) * 2);
            granularity = std::max(1u, count / numJobs);
        }

        // ジョブ数を計算
        uint32_t numJobs = (count + granularity - 1) / granularity;
        auto counter = std::make_shared<JobCounter>(numJobs);

        // 各範囲をジョブとして投入
        for (uint32_t i = 0; i < numJobs; ++i) {
            uint32_t jobBegin = begin + i * granularity;
            uint32_t jobEnd = std::min(jobBegin + granularity, end);

            Submit([func, jobBegin, jobEnd] {
                func(jobBegin, jobEnd);
            }, counter, JobPriority::Normal);
        }

        // 全ジョブの完了を待機
        counter->Wait();
    }

    [[nodiscard]] uint32_t GetWorkerCount() const noexcept
    {
        return static_cast<uint32_t>(workers_.size());
    }

    [[nodiscard]] bool IsWorkerThread() const noexcept
    {
        auto thisId = std::this_thread::get_id();
        for (const auto& worker : workers_) {
            if (worker.get_id() == thisId) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] uint32_t GetPendingJobCount() const noexcept
    {
        return pendingJobs_.load(std::memory_order_acquire);
    }

private:
    void WorkerThread(uint32_t workerId)
    {
        // スレッド名を設定（デバッグ用）
        #if defined(_WIN32) && defined(_DEBUG)
            std::wstring name = L"JobWorker_" + std::to_wstring(workerId);
            SetThreadDescription(GetCurrentThread(), name.c_str());
        #else
            (void)workerId;
        #endif

        while (true) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mutex_);

                // ジョブが来るか、シャットダウンされるまで待機
                condition_.wait(lock, [this] {
                    return !running_ || HasPendingJobs();
                });

                if (!running_ && !HasPendingJobs()) {
                    return;
                }

                // 優先度順にジョブを取得
                if (!TryPopJob(job)) {
                    continue;
                }

                --pendingJobs_;
            }

            // ジョブを実行
            if (job.function) {
                job.function();
            }

            // カウンターをデクリメント
            if (job.counter) {
                job.counter->Decrement();
            }
        }
    }

    [[nodiscard]] bool HasPendingJobs() const noexcept
    {
        for (int i = 0; i < static_cast<int>(JobPriority::Count); ++i) {
            if (!queues_[i].empty()) {
                return true;
            }
        }
        return false;
    }

    bool TryPopJob(Job& outJob)
    {
        // 優先度順（High → Normal → Low）でジョブを取得
        for (int i = 0; i < static_cast<int>(JobPriority::Count); ++i) {
            if (!queues_[i].empty()) {
                outJob = std::move(queues_[i].front());
                queues_[i].pop_front();
                return true;
            }
        }
        return false;
    }

    std::vector<std::thread> workers_;
    std::deque<Job> queues_[static_cast<int>(JobPriority::Count)];
    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<uint32_t> pendingJobs_{0};
    bool running_ = false;
};

//----------------------------------------------------------------------------
// JobSystem シングルトン
//----------------------------------------------------------------------------

void JobSystem::Create(uint32_t numWorkers)
{
    if (!instance_) {
        instance_ = std::unique_ptr<JobSystem>(new JobSystem());
        instance_->Initialize(numWorkers);
    }
}

void JobSystem::Destroy()
{
    if (instance_) {
        instance_->Shutdown();
        instance_.reset();
    }
}

JobSystem::~JobSystem() = default;

void JobSystem::Initialize(uint32_t numWorkers)
{
    impl_ = std::make_unique<Impl>();
    impl_->Initialize(numWorkers);
}

void JobSystem::Shutdown()
{
    if (impl_) {
        impl_->Shutdown();
        impl_.reset();
    }
}

//----------------------------------------------------------------------------
// ジョブ投入
//----------------------------------------------------------------------------

void JobSystem::Submit(JobFunction job, JobPriority priority)
{
    impl_->Submit(std::move(job), priority);
}

void JobSystem::Submit(JobFunction job, JobCounterPtr counter, JobPriority priority)
{
    impl_->Submit(std::move(job), std::move(counter), priority);
}

JobCounterPtr JobSystem::SubmitAndGetCounter(JobFunction job, JobPriority priority)
{
    return impl_->SubmitAndGetCounter(std::move(job), priority);
}

//----------------------------------------------------------------------------
// 並列ループ
//----------------------------------------------------------------------------

void JobSystem::ParallelFor(uint32_t begin, uint32_t end,
                            const std::function<void(uint32_t)>& func,
                            uint32_t granularity)
{
    impl_->ParallelFor(begin, end, func, granularity);
}

void JobSystem::ParallelForRange(uint32_t begin, uint32_t end,
                                 const std::function<void(uint32_t, uint32_t)>& func,
                                 uint32_t granularity)
{
    impl_->ParallelForRange(begin, end, func, granularity);
}

//----------------------------------------------------------------------------
// 状態取得
//----------------------------------------------------------------------------

uint32_t JobSystem::GetWorkerCount() const noexcept
{
    return impl_ ? impl_->GetWorkerCount() : 0;
}

bool JobSystem::IsWorkerThread() const noexcept
{
    return impl_ ? impl_->IsWorkerThread() : false;
}

uint32_t JobSystem::GetPendingJobCount() const noexcept
{
    return impl_ ? impl_->GetPendingJobCount() : 0;
}
