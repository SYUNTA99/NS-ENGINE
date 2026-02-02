//----------------------------------------------------------------------------
//! @file   system_graph.h
//! @brief  ECS SystemGraph - System依存関係グラフとトポロジカルソート
//----------------------------------------------------------------------------
#pragma once

#include "system.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>
#include <typeindex>
#include <cassert>

#if defined(_DEBUG)
#include "common/logging/logging.h"
#endif

namespace ECS {

//! SystemのユニークID（型名ハッシュ）
using SystemId = std::type_index;

//============================================================================
//! @brief System依存関係情報（システム実体は含まない）
//============================================================================
struct SystemNodeInfo {
    SystemId id{typeid(void)};              //!< System ID
    int priority = 0;                       //!< 優先度（依存関係がない場合のソート用）
    std::vector<SystemId> runAfter;         //!< このSystemの後に実行
    std::vector<SystemId> runBefore;        //!< このSystemの前に実行
    const char* name = "Unknown";           //!< System名（デバッグ用）
};

//============================================================================
//! @brief System登録情報（CommitSystem用、システム実体を含む）
//============================================================================
struct SystemEntry {
    SystemId id{typeid(void)};
    std::unique_ptr<ISystem> system;
    int priority = 0;
    std::vector<SystemId> runAfter;
    std::vector<SystemId> runBefore;
    const char* name = "Unknown";
};

//============================================================================
//! @brief RenderSystem登録情報
//============================================================================
struct RenderSystemEntry {
    SystemId id{typeid(void)};
    std::unique_ptr<IRenderSystem> system;
    int priority = 0;
    std::vector<SystemId> runAfter;
    std::vector<SystemId> runBefore;
    const char* name = "Unknown";
};

//============================================================================
//! @brief 依存関係グラフ
//!
//! Systemの依存関係をグラフで管理し、トポロジカルソートで実行順序を決定。
//! Kahnのアルゴリズムで循環依存を検出。
//!
//! @note システム実体（unique_ptr）は保持しない。IDと依存関係のみを管理。
//============================================================================
class SystemGraph {
public:
    //------------------------------------------------------------------------
    //! @brief ノードを追加（依存関係情報のみ）
    //------------------------------------------------------------------------
    void AddNode(SystemId id, int priority,
                 const std::vector<SystemId>& runAfter,
                 const std::vector<SystemId>& runBefore,
                 const char* name)
    {
        SystemNodeInfo info;
        info.id = id;
        info.priority = priority;
        info.runAfter = runAfter;
        info.runBefore = runBefore;
        info.name = name;

        nodes_[id] = std::move(info);
        adjacency_[id];  // 空のリストを作成
        inDegree_[id] = 0;
        dirty_ = true;
    }

    //------------------------------------------------------------------------
    //! @brief 全ての依存エッジを構築
    //------------------------------------------------------------------------
    void BuildEdges() {
        // エッジをクリア
        for (auto& [id, _] : adjacency_) {
            adjacency_[id].clear();
        }
        for (auto& [id, _] : inDegree_) {
            inDegree_[id] = 0;
        }
        pendingEdges_.clear();

        for (auto& [id, info] : nodes_) {
            // runAfter: thisはdepより後に実行 -> dep -> this
            for (SystemId dep : info.runAfter) {
                if (nodes_.find(dep) != nodes_.end()) {
                    adjacency_[dep].push_back(id);
                    ++inDegree_[id];
                } else {
                    pendingEdges_.push_back({dep, id});
                }
            }

            // runBefore: thisはdepより前に実行 -> this -> dep
            for (SystemId dep : info.runBefore) {
                if (nodes_.find(dep) != nodes_.end()) {
                    adjacency_[id].push_back(dep);
                    ++inDegree_[dep];
                } else {
                    pendingEdges_.push_back({id, dep});
                }
            }
        }

        // 保留中のエッジを解決
        ResolvePendingEdges();
    }

    //------------------------------------------------------------------------
    //! @brief トポロジカルソートを実行
    //! @return ソート済みSystemIdリスト（循環時は空リスト）
    //------------------------------------------------------------------------
    [[nodiscard]] std::vector<SystemId> TopologicalSort() {
        if (dirty_) {
            BuildEdges();
            dirty_ = false;
        }

        // 入次数のコピー（破壊的操作のため）
        auto inDegree = inDegree_;

        // 入次数0のノードを優先度順にキュー
        auto cmp = [this](SystemId a, SystemId b) {
            return nodes_[a].priority > nodes_[b].priority;  // 小さい方が先
        };
        std::priority_queue<SystemId, std::vector<SystemId>, decltype(cmp)> queue(cmp);

        // 初期キュー構築
        for (const auto& [id, _] : nodes_) {
            if (inDegree[id] == 0) {
                queue.push(id);
            }
        }

        std::vector<SystemId> sorted;
        sorted.reserve(nodes_.size());

        while (!queue.empty()) {
            SystemId current = queue.top();
            queue.pop();
            sorted.push_back(current);

            // 隣接ノードの入次数を減らす
            for (SystemId neighbor : adjacency_[current]) {
                if (--inDegree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }

        // 循環検出
        if (sorted.size() != nodes_.size()) {
#if defined(_DEBUG)
            ReportCyclicDependency();
#endif
            return {};  // 循環あり
        }

        return sorted;
    }

    //------------------------------------------------------------------------
    //! @brief ノードが存在するか確認
    //------------------------------------------------------------------------
    [[nodiscard]] bool HasNode(SystemId id) const noexcept {
        return nodes_.find(id) != nodes_.end();
    }

    //------------------------------------------------------------------------
    //! @brief ノード数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t NodeCount() const noexcept {
        return nodes_.size();
    }

    //------------------------------------------------------------------------
    //! @brief グラフをクリア
    //------------------------------------------------------------------------
    void Clear() {
        nodes_.clear();
        adjacency_.clear();
        inDegree_.clear();
        pendingEdges_.clear();
        dirty_ = true;
    }

private:
    //------------------------------------------------------------------------
    //! @brief 保留中のエッジを解決
    //------------------------------------------------------------------------
    void ResolvePendingEdges() {
        auto it = pendingEdges_.begin();
        while (it != pendingEdges_.end()) {
            if (nodes_.find(it->first) != nodes_.end() &&
                nodes_.find(it->second) != nodes_.end()) {
                adjacency_[it->first].push_back(it->second);
                ++inDegree_[it->second];
                it = pendingEdges_.erase(it);
            } else {
                ++it;
            }
        }
    }

#if defined(_DEBUG)
    //------------------------------------------------------------------------
    //! @brief 循環依存をレポート
    //------------------------------------------------------------------------
    void ReportCyclicDependency() {
        std::vector<SystemId> path;
        std::unordered_set<SystemId> visiting;
        std::unordered_set<SystemId> visited;

        for (const auto& [id, _] : nodes_) {
            if (FindCycle(id, visiting, visited, path)) {
                std::string cycleStr;
                for (size_t i = 0; i < path.size(); ++i) {
                    cycleStr += nodes_[path[i]].name;
                    if (i < path.size() - 1) {
                        cycleStr += " -> ";
                    }
                }
                LOG_ERROR(std::format("[ECS] Cyclic dependency detected: {}", cycleStr));
                return;
            }
        }
    }

    bool FindCycle(
        SystemId node,
        std::unordered_set<SystemId>& visiting,
        std::unordered_set<SystemId>& visited,
        std::vector<SystemId>& path)
    {
        if (visited.count(node)) return false;
        if (visiting.count(node)) {
            path.push_back(node);
            return true;
        }

        visiting.insert(node);
        path.push_back(node);

        auto it = adjacency_.find(node);
        if (it != adjacency_.end()) {
            for (SystemId neighbor : it->second) {
                if (FindCycle(neighbor, visiting, visited, path)) {
                    return true;
                }
            }
        }

        path.pop_back();
        visiting.erase(node);
        visited.insert(node);
        return false;
    }
#endif

private:
    std::unordered_map<SystemId, SystemNodeInfo> nodes_;
    std::unordered_map<SystemId, std::vector<SystemId>> adjacency_;
    std::unordered_map<SystemId, int> inDegree_;
    std::vector<std::pair<SystemId, SystemId>> pendingEdges_;
    bool dirty_ = true;
};

//============================================================================
//! @brief RenderSystem用の依存関係グラフ
//============================================================================
class RenderSystemGraph {
public:
    void AddNode(SystemId id, int priority,
                 const std::vector<SystemId>& runAfter,
                 const std::vector<SystemId>& runBefore,
                 const char* name)
    {
        SystemNodeInfo info;
        info.id = id;
        info.priority = priority;
        info.runAfter = runAfter;
        info.runBefore = runBefore;
        info.name = name;

        nodes_[id] = std::move(info);
        adjacency_[id];
        inDegree_[id] = 0;
        dirty_ = true;
    }

    void BuildEdges() {
        for (auto& [id, _] : adjacency_) {
            adjacency_[id].clear();
        }
        for (auto& [id, _] : inDegree_) {
            inDegree_[id] = 0;
        }
        pendingEdges_.clear();

        for (auto& [id, info] : nodes_) {
            for (SystemId dep : info.runAfter) {
                if (nodes_.find(dep) != nodes_.end()) {
                    adjacency_[dep].push_back(id);
                    ++inDegree_[id];
                } else {
                    pendingEdges_.push_back({dep, id});
                }
            }
            for (SystemId dep : info.runBefore) {
                if (nodes_.find(dep) != nodes_.end()) {
                    adjacency_[id].push_back(dep);
                    ++inDegree_[dep];
                } else {
                    pendingEdges_.push_back({id, dep});
                }
            }
        }
        ResolvePendingEdges();
    }

    [[nodiscard]] std::vector<SystemId> TopologicalSort() {
        if (dirty_) {
            BuildEdges();
            dirty_ = false;
        }

        auto inDegree = inDegree_;
        auto cmp = [this](SystemId a, SystemId b) {
            return nodes_[a].priority > nodes_[b].priority;
        };
        std::priority_queue<SystemId, std::vector<SystemId>, decltype(cmp)> queue(cmp);

        for (const auto& [id, _] : nodes_) {
            if (inDegree[id] == 0) {
                queue.push(id);
            }
        }

        std::vector<SystemId> sorted;
        sorted.reserve(nodes_.size());

        while (!queue.empty()) {
            SystemId current = queue.top();
            queue.pop();
            sorted.push_back(current);

            for (SystemId neighbor : adjacency_[current]) {
                if (--inDegree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }

        if (sorted.size() != nodes_.size()) {
#if defined(_DEBUG)
            ReportCyclicDependency();
#endif
            return {};
        }

        return sorted;
    }

    [[nodiscard]] bool HasNode(SystemId id) const noexcept {
        return nodes_.find(id) != nodes_.end();
    }

    [[nodiscard]] size_t NodeCount() const noexcept {
        return nodes_.size();
    }

    void Clear() {
        nodes_.clear();
        adjacency_.clear();
        inDegree_.clear();
        pendingEdges_.clear();
        dirty_ = true;
    }

private:
    void ResolvePendingEdges() {
        auto it = pendingEdges_.begin();
        while (it != pendingEdges_.end()) {
            if (nodes_.find(it->first) != nodes_.end() &&
                nodes_.find(it->second) != nodes_.end()) {
                adjacency_[it->first].push_back(it->second);
                ++inDegree_[it->second];
                it = pendingEdges_.erase(it);
            } else {
                ++it;
            }
        }
    }

#if defined(_DEBUG)
    //------------------------------------------------------------------------
    //! @brief 循環依存をレポート
    //------------------------------------------------------------------------
    void ReportCyclicDependency() {
        std::vector<SystemId> path;
        std::unordered_set<SystemId> visiting;
        std::unordered_set<SystemId> visited;

        for (const auto& [id, _] : nodes_) {
            if (FindCycle(id, visiting, visited, path)) {
                std::string cycleStr;
                for (size_t i = 0; i < path.size(); ++i) {
                    cycleStr += nodes_[path[i]].name;
                    if (i < path.size() - 1) {
                        cycleStr += " -> ";
                    }
                }
                LOG_ERROR(std::format("[ECS] Cyclic dependency detected in RenderSystem graph: {}", cycleStr));
                return;
            }
        }
    }

    bool FindCycle(
        SystemId node,
        std::unordered_set<SystemId>& visiting,
        std::unordered_set<SystemId>& visited,
        std::vector<SystemId>& path)
    {
        if (visited.count(node)) return false;
        if (visiting.count(node)) {
            path.push_back(node);
            return true;
        }

        visiting.insert(node);
        path.push_back(node);

        auto it = adjacency_.find(node);
        if (it != adjacency_.end()) {
            for (SystemId neighbor : it->second) {
                if (FindCycle(neighbor, visiting, visited, path)) {
                    return true;
                }
            }
        }

        path.pop_back();
        visiting.erase(node);
        visited.insert(node);
        return false;
    }
#endif

private:
    std::unordered_map<SystemId, SystemNodeInfo> nodes_;
    std::unordered_map<SystemId, std::vector<SystemId>> adjacency_;
    std::unordered_map<SystemId, int> inDegree_;
    std::vector<std::pair<SystemId, SystemId>> pendingEdges_;
    bool dirty_ = true;
};

} // namespace ECS
