//----------------------------------------------------------------------------
//! @file   component_storage.h
//! @brief  ECS ComponentStorage - SoA配列によるコンポーネント格納
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include "common/utility/non_copyable.h"
#include <vector>
#include <unordered_map>
#include <cassert>

namespace ECS {

//============================================================================
//! @brief コンポーネントストレージの基底インターフェース
//!
//! 型消去のための基底クラス。WorldがComponentStorage<T>を
//! 統一的に管理するために使用する。
//============================================================================
class IComponentStorageBase {
public:
    virtual ~IComponentStorageBase() = default;

    //! エンティティ破棄時にコンポーネントを削除
    virtual void OnEntityDestroyed(Actor e) = 0;

    //! 全データクリア
    virtual void Clear() = 0;

    //! 格納数を取得
    [[nodiscard]] virtual size_t Size() const noexcept = 0;
};

//============================================================================
//! @brief コンポーネントストレージ（SoA配列）
//!
//! 同じ型のコンポーネントを連続したメモリに格納する。
//! - data_: コンポーネントデータの連続配列（キャッシュ効率◎）
//! - entities_: 各データに対応するEntity
//! - indices_: Actor → data_インデックスの逆引き
//!
//! @tparam T コンポーネントの型（POD推奨）
//============================================================================
template<typename T>
class ComponentStorage : public IComponentStorageBase, private NonCopyable {
public:
    ComponentStorage() = default;
    ~ComponentStorage() override = default;

    //------------------------------------------------------------------------
    //! @brief コンポーネントを追加
    //! @param e 追加先のエンティティ
    //! @param args コンストラクタ引数
    //! @return 追加されたコンポーネントへのポインタ
    //------------------------------------------------------------------------
    template<typename... Args>
    T* Add(Actor e, Args&&... args) {
        assert(e.IsValid() && "Cannot add component to invalid entity");
        assert(!Has(e) && "Actor already has this component");

        size_t index = data_.size();
        indices_[e] = index;
        entities_.push_back(e);
        data_.emplace_back(std::forward<Args>(args)...);

        return &data_.back();
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを取得
    //! @param e 対象のエンティティ
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    [[nodiscard]] T* Get(Actor e) {
        auto it = indices_.find(e);
        if (it == indices_.end()) {
            return nullptr;
        }
        return &data_[it->second];
    }

    [[nodiscard]] const T* Get(Actor e) const {
        auto it = indices_.find(e);
        if (it == indices_.end()) {
            return nullptr;
        }
        return &data_[it->second];
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを所持しているか確認
    //! @param e 対象のエンティティ
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool Has(Actor e) const {
        return indices_.find(e) != indices_.end();
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを削除
    //! @param e 対象のエンティティ
    //------------------------------------------------------------------------
    void Remove(Actor e) {
        auto it = indices_.find(e);
        if (it == indices_.end()) {
            return;
        }

        size_t indexToRemove = it->second;
        size_t lastIndex = data_.size() - 1;

        if (indexToRemove != lastIndex) {
            // 末尾要素を削除位置に移動（swap and pop）
            data_[indexToRemove] = std::move(data_[lastIndex]);
            entities_[indexToRemove] = entities_[lastIndex];

            // 移動した要素のインデックスを更新
            indices_[entities_[indexToRemove]] = indexToRemove;
        }

        // 末尾を削除
        data_.pop_back();
        entities_.pop_back();
        indices_.erase(it);
    }

    //------------------------------------------------------------------------
    //! @brief エンティティ破棄時のコールバック
    //! @param e 破棄されたエンティティ
    //------------------------------------------------------------------------
    void OnEntityDestroyed(Actor e) override {
        Remove(e);
    }

    //------------------------------------------------------------------------
    //! @brief 全データクリア
    //------------------------------------------------------------------------
    void Clear() override {
        data_.clear();
        entities_.clear();
        indices_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief 格納数を取得
    //! @return 格納されているコンポーネント数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Size() const noexcept override {
        return data_.size();
    }

    //------------------------------------------------------------------------
    //! @brief 全コンポーネントに対して処理を実行（高速）
    //! @tparam Func 処理関数の型
    //! @param func 各コンポーネントに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEach(Func&& func) {
        for (auto& component : data_) {
            func(component);
        }
    }

    template<typename Func>
    void ForEach(Func&& func) const {
        for (const auto& component : data_) {
            func(component);
        }
    }

    //------------------------------------------------------------------------
    //! @brief Entity付きで全コンポーネントに対して処理を実行
    //! @tparam Func 処理関数の型
    //! @param func 各エンティティとコンポーネントに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEachWithEntity(Func&& func) {
        for (size_t i = 0; i < data_.size(); ++i) {
            func(entities_[i], data_[i]);
        }
    }

    template<typename Func>
    void ForEachWithEntity(Func&& func) const {
        for (size_t i = 0; i < data_.size(); ++i) {
            func(entities_[i], data_[i]);
        }
    }

    //------------------------------------------------------------------------
    //! @brief 生データ配列への直接アクセス（SIMD等の最適化用）
    //! @return データ配列への参照
    //------------------------------------------------------------------------
    [[nodiscard]] std::vector<T>& GetRawData() noexcept {
        return data_;
    }

    [[nodiscard]] const std::vector<T>& GetRawData() const noexcept {
        return data_;
    }

    //------------------------------------------------------------------------
    //! @brief エンティティ配列への直接アクセス
    //! @return エンティティ配列への参照
    //------------------------------------------------------------------------
    [[nodiscard]] const std::vector<Actor>& GetEntities() const noexcept {
        return entities_;
    }

private:
    std::vector<T> data_;                          //!< コンポーネントデータ（連続メモリ）
    std::vector<Actor> entities_;                 //!< data_[i]に対応するActor
    std::unordered_map<Actor, size_t> indices_;   //!< Actor → data_インデックス
};

} // namespace ECS
