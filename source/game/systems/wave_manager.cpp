//----------------------------------------------------------------------------
//! @file   wave_manager.cpp
//! @brief  ウェーブ管理システム実装
//----------------------------------------------------------------------------
#include "wave_manager.h"
#include "game/entities/group.h"
#include "common/logging/logging.h"
#include <algorithm>

//----------------------------------------------------------------------------
void WaveManager::Initialize(const std::vector<WaveData>& waves)
{
    waves_ = waves;
    currentWave_ = 1;
    currentWaveGroups_.clear();
    waveCleared_ = false;
    waveTransitionTimer_ = 0.0f;

    LOG_INFO("[WaveManager] Initialized with " + std::to_string(waves_.size()) + " waves");
}

//----------------------------------------------------------------------------
void WaveManager::Update()
{
    if (waves_.empty()) {
        LOG_DEBUG("[WaveManager] Update: waves empty");
        return;
    }
    if (IsAllWavesCleared()) return;
    if (isTransitioning_) return;  // トランジション中はクリア判定しない

    // 現在ウェーブのクリア判定
    bool cleared = IsCurrentWaveCleared();
    if (!waveCleared_ && cleared) {
        LOG_INFO("[WaveManager] Wave " + std::to_string(currentWave_) + " cleared detected!");
        waveCleared_ = true;

        LOG_INFO("[WaveManager] Wave " + std::to_string(currentWave_) + " cleared!");

        if (onWaveCleared_) {
            onWaveCleared_(currentWave_);
        }

        // 全ウェーブクリアか確認
        if (currentWave_ >= static_cast<int>(waves_.size())) {
            LOG_INFO("[WaveManager] All waves cleared!");
            if (onAllWavesCleared_) {
                onAllWavesCleared_();
            }
        } else {
            // 次のウェーブへトランジション開始
            StartTransition();
        }
    }
}

//----------------------------------------------------------------------------
void WaveManager::Reset()
{
    currentWave_ = 1;
    currentWaveGroups_.clear();
    waveCleared_ = false;
    waveTransitionTimer_ = 0.0f;

    LOG_INFO("[WaveManager] Reset");
}

//----------------------------------------------------------------------------
void WaveManager::SpawnCurrentWave()
{
    if (waves_.empty()) {
        LOG_WARN("[WaveManager] No waves to spawn");
        return;
    }

    int waveIndex = currentWave_ - 1;
    if (waveIndex < 0 || waveIndex >= static_cast<int>(waves_.size())) {
        LOG_WARN("[WaveManager] Invalid wave index: " + std::to_string(waveIndex));
        return;
    }

    const WaveData& wave = waves_[waveIndex];
    currentWaveGroups_.clear();
    waveCleared_ = false;

    LOG_INFO("[WaveManager] Spawning wave " + std::to_string(currentWave_) +
             " (" + std::to_string(wave.groups.size()) + " groups)");

    // グループスポーナーが設定されていればそれを使用
    if (groupSpawner_) {
        for (const GroupData& gd : wave.groups) {
            Group* group = groupSpawner_(gd);
            if (group) {
                currentWaveGroups_.push_back(group);
                LOG_INFO("[WaveManager] Registered group: " + gd.id);
            }
        }
    }
    LOG_INFO("[WaveManager] Wave " + std::to_string(currentWave_) + " spawned with " +
             std::to_string(currentWaveGroups_.size()) + " groups");
}

//----------------------------------------------------------------------------
void WaveManager::AdvanceToNextWave()
{
    if (currentWave_ >= static_cast<int>(waves_.size())) {
        LOG_WARN("[WaveManager] Already at last wave");
        return;
    }

    currentWave_++;
    waveCleared_ = false;
    currentWaveGroups_.clear();

    LOG_INFO("[WaveManager] Advanced to wave " + std::to_string(currentWave_));

    // 次のウェーブをスポーン
    SpawnCurrentWave();
}

//----------------------------------------------------------------------------
bool WaveManager::IsCurrentWaveCleared() const
{
    if (currentWaveGroups_.empty()) {
        LOG_DEBUG("[WaveManager] IsCurrentWaveCleared: no groups registered");
        return true;
    }

    // 全グループが全滅または味方化していればクリア
    int aliveEnemyCount = 0;
    for (Group* group : currentWaveGroups_) {
        if (!group) continue;
        if (group->IsDefeated()) continue;
        if (group->IsAlly()) continue;

        // 敵として生存しているグループがある
        aliveEnemyCount++;
    }

    if (aliveEnemyCount > 0) {
        return false;
    }

    LOG_INFO("[WaveManager] All groups in wave defeated or allied");
    return true;
}

//----------------------------------------------------------------------------
bool WaveManager::IsAllWavesCleared() const
{
    // 最終ウェーブまで到達していてクリア済み
    return currentWave_ >= static_cast<int>(waves_.size()) && waveCleared_;
}

//----------------------------------------------------------------------------
void WaveManager::RegisterGroup(Group* group)
{
    if (!group) return;

    auto it = std::find(currentWaveGroups_.begin(), currentWaveGroups_.end(), group);
    if (it == currentWaveGroups_.end()) {
        currentWaveGroups_.push_back(group);
    }
}

//----------------------------------------------------------------------------
void WaveManager::UnregisterGroup(Group* group)
{
    auto it = std::find(currentWaveGroups_.begin(), currentWaveGroups_.end(), group);
    if (it != currentWaveGroups_.end()) {
        currentWaveGroups_.erase(it);
    }
}

//----------------------------------------------------------------------------
void WaveManager::ClearGroups()
{
    currentWaveGroups_.clear();
}

//----------------------------------------------------------------------------
float WaveManager::GetCurrentWaveCameraY() const
{
    // Wave 1 = 最下部エリア、Wave 2 = 中央、Wave 3 = 最上部
    // Y座標は下から上へ減少（画面座標系）
    int totalWaves = static_cast<int>(waves_.size());
    if (totalWaves <= 0) return areaHeight_ * 0.5f;

    // 現在ウェーブのエリア中央Y
    // Wave 1: (totalWaves - 1) * areaHeight + areaHeight/2
    // Wave 2: (totalWaves - 2) * areaHeight + areaHeight/2
    // Wave N: (totalWaves - N) * areaHeight + areaHeight/2
    float waveIndex = static_cast<float>(totalWaves - currentWave_);
    return waveIndex * areaHeight_ + areaHeight_ * 0.5f;
}

//----------------------------------------------------------------------------
void WaveManager::StartTransition()
{
    if (isTransitioning_) return;
    if (currentWave_ >= static_cast<int>(waves_.size())) return;

    isTransitioning_ = true;
    transitionProgress_ = 0.0f;
    startCameraY_ = GetCurrentWaveCameraY();

    // 次ウェーブの目標Y座標を計算
    int nextWave = currentWave_ + 1;
    int totalWaves = static_cast<int>(waves_.size());
    float waveIndex = static_cast<float>(totalWaves - nextWave);
    targetCameraY_ = waveIndex * areaHeight_ + areaHeight_ * 0.5f;

    LOG_INFO("[WaveManager] Transition started: Y " + std::to_string(startCameraY_) +
             " -> " + std::to_string(targetCameraY_));
}

//----------------------------------------------------------------------------
void WaveManager::UpdateTransition(float dt)
{
    if (!isTransitioning_) return;

    transitionProgress_ += dt / transitionDuration_;

    if (transitionProgress_ >= 1.0f) {
        transitionProgress_ = 1.0f;
        isTransitioning_ = false;

        LOG_INFO("[WaveManager] Transition complete");

        // 次ウェーブへ進む
        AdvanceToNextWave();

        if (onTransitionComplete_) {
            onTransitionComplete_();
        }
    }
}
