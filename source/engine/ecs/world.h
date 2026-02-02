//----------------------------------------------------------------------------
//! @file   world.h
//! @brief  ECS World - アクター・コンポーネント・システムの中央管理
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include "actor_manager.h"
#include "component_storage.h"
#include "system.h"
#include "common/utility/non_copyable.h"
#include <memory>
#include <vector>
#include <typeindex>
#include <algorithm>
#include <type_traits>
#include <string>

// 前方宣言
class GameObject;

namespace ECS {

//============================================================================
//! @brief ECSワールド
//!
//! アクター、コンポーネント、システムを一元管理する中央データストア。
//! - World-centric API: world.AddComponent<T>(actor, ...)
//! - SoA配列による高速イテレーション
//! - クラスベースのSystem登録: world.RegisterSystem<T>()
//============================================================================
class World : private NonCopyable {
public:

    World();   // unique_ptr<GameObject>のためcppで定義
    ~World();  // unique_ptr<GameObject>のためcppで定義

    //========================================================================
    // Actor管理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 新しいアクターを生成
    //! @return 生成されたアクター
    //------------------------------------------------------------------------
    [[nodiscard]] Actor CreateActor() {
        return entities_.Create();
    }

    //------------------------------------------------------------------------
    //! @brief アクターを破棄
    //! @param actor 破棄するアクター
    //------------------------------------------------------------------------
    void DestroyActor(Actor actor) {
        if (!entities_.IsAlive(actor)) {
            return;
        }

        // 全ストレージからコンポーネントを削除
        for (auto& [typeIndex, storage] : storages_) {
            storage->OnEntityDestroyed(actor);
        }

        entities_.Destroy(actor);
    }

    //------------------------------------------------------------------------
    //! @brief アクターが生存しているか確認
    //! @param actor 確認するアクター
    //! @return 生存している場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsAlive(Actor actor) const {
        return entities_.IsAlive(actor);
    }

    //------------------------------------------------------------------------
    //! @brief 生存アクター数を取得
    //! @return 生存アクター数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t ActorCount() const noexcept {
        return entities_.Count();
    }

    //========================================================================
    // Component管理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief コンポーネントを追加
    //! @tparam T コンポーネントの型
    //! @tparam Args コンストラクタ引数の型
    //! @param actor 追加先のアクター
    //! @param args コンストラクタ引数
    //! @return 追加されたコンポーネントへのポインタ
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    T* AddComponent(Actor actor, Args&&... args) {
        if (!entities_.IsAlive(actor)) {
            return nullptr;
        }
        auto& storage = GetOrCreateStorage<T>();
        return storage.Add(actor, std::forward<Args>(args)...);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを取得
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponent(Actor actor) {
        auto* storage = GetStorage<T>();
        if (!storage) {
            return nullptr;
        }
        return storage->Get(actor);
    }

    template<typename T>
    [[nodiscard]] const T* GetComponent(Actor actor) const {
        auto* storage = GetStorage<T>();
        if (!storage) {
            return nullptr;
        }
        return storage->Get(actor);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを所持しているか確認
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasComponent(Actor actor) const {
        auto* storage = GetStorage<T>();
        if (!storage) {
            return false;
        }
        return storage->Has(actor);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを削除
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //------------------------------------------------------------------------
    template<typename T>
    void RemoveComponent(Actor actor) {
        auto* storage = GetStorage<T>();
        if (storage) {
            storage->Remove(actor);
        }
    }

    //========================================================================
    // バッチクエリ
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 単一コンポーネントに対してイテレーション
    //! @tparam T コンポーネントの型
    //! @tparam Func 処理関数の型
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename T, typename Func>
    void ForEach(Func&& func) {
        auto* storage = GetStorage<T>();
        if (!storage) {
            return;
        }
        storage->ForEachWithEntity([this, &func](Actor e, T& component) {
            if (entities_.IsAlive(e)) {
                func(e, component);
            }
        });
    }

    //------------------------------------------------------------------------
    //! @brief 2つのコンポーネントを持つアクターに対してイテレーション
    //! @tparam T1, T2 コンポーネントの型
    //! @tparam Func 処理関数の型
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename T1, typename T2, typename Func>
    void ForEach(Func&& func) {
        auto* storage1 = GetStorage<T1>();
        auto* storage2 = GetStorage<T2>();
        if (!storage1 || !storage2) {
            return;
        }

        // 小さい方をイテレート（最適化）
        if (storage1->Size() <= storage2->Size()) {
            storage1->ForEachWithEntity([this, storage2, &func](Actor e, T1& c1) {
                if (!entities_.IsAlive(e)) return;
                T2* c2 = storage2->Get(e);
                if (c2) {
                    func(e, c1, *c2);
                }
            });
        } else {
            storage2->ForEachWithEntity([this, storage1, &func](Actor e, T2& c2) {
                if (!entities_.IsAlive(e)) return;
                T1* c1 = storage1->Get(e);
                if (c1) {
                    func(e, *c1, c2);
                }
            });
        }
    }

    //------------------------------------------------------------------------
    //! @brief 3つのコンポーネントを持つアクターに対してイテレーション
    //! @tparam T1, T2, T3 コンポーネントの型
    //! @tparam Func 処理関数の型
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename T1, typename T2, typename T3, typename Func>
    void ForEach(Func&& func) {
        auto* storage1 = GetStorage<T1>();
        auto* storage2 = GetStorage<T2>();
        auto* storage3 = GetStorage<T3>();
        if (!storage1 || !storage2 || !storage3) {
            return;
        }

        // 最小のストレージをイテレート
        size_t minSize = (std::min)({storage1->Size(), storage2->Size(), storage3->Size()});

        if (storage1->Size() == minSize) {
            storage1->ForEachWithEntity([this, storage2, storage3, &func](Actor e, T1& c1) {
                if (!entities_.IsAlive(e)) return;
                T2* c2 = storage2->Get(e);
                T3* c3 = storage3->Get(e);
                if (c2 && c3) {
                    func(e, c1, *c2, *c3);
                }
            });
        } else if (storage2->Size() == minSize) {
            storage2->ForEachWithEntity([this, storage1, storage3, &func](Actor e, T2& c2) {
                if (!entities_.IsAlive(e)) return;
                T1* c1 = storage1->Get(e);
                T3* c3 = storage3->Get(e);
                if (c1 && c3) {
                    func(e, *c1, c2, *c3);
                }
            });
        } else {
            storage3->ForEachWithEntity([this, storage1, storage2, &func](Actor e, T3& c3) {
                if (!entities_.IsAlive(e)) return;
                T1* c1 = storage1->Get(e);
                T2* c2 = storage2->Get(e);
                if (c1 && c2) {
                    func(e, *c1, *c2, c3);
                }
            });
        }
    }

    //========================================================================
    // System管理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 更新Systemをクラスベースで登録
    //! @tparam T ISystemを継承したクラス（finalを推奨）
    //------------------------------------------------------------------------
    template<typename T>
    void RegisterSystem() {
        static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from ISystem");
        auto system = std::make_unique<T>();
        int priority = system->Priority();
        systems_.emplace_back(priority, std::move(system));
        SortSystems();
    }

    //------------------------------------------------------------------------
    //! @brief 描画Systemをクラスベースで登録
    //! @tparam T IRenderSystemを継承したクラス（finalを推奨）
    //------------------------------------------------------------------------
    template<typename T>
    void RegisterRenderSystem() {
        static_assert(std::is_base_of_v<IRenderSystem, T>, "T must inherit from IRenderSystem");
        auto system = std::make_unique<T>();
        int priority = system->Priority();
        renderSystems_.emplace_back(priority, std::move(system));
        SortRenderSystems();
    }

    //========================================================================
    // フレーム処理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 固定タイムステップ更新
    //! @param dt 固定デルタタイム（通常1/60秒）
    //------------------------------------------------------------------------
    void FixedUpdate(float dt) {
        for (auto& [priority, system] : systems_) {
            system->Execute(*this, dt);
        }
    }

    //------------------------------------------------------------------------
    //! @brief 描画
    //! @param alpha 補間係数（0.0〜1.0）
    //------------------------------------------------------------------------
    void Render(float alpha) {
        for (auto& [priority, renderSystem] : renderSystems_) {
            renderSystem->Render(*this, alpha);
        }
    }

    //========================================================================
    // OOP API（従来のGameObjectベース）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief ゲームオブジェクトを生成
    //! @param name オブジェクト名
    //! @return 生成されたゲームオブジェクトへのポインタ
    //------------------------------------------------------------------------
    GameObject* CreateGameObject(const std::string& name = "GameObject");

    //------------------------------------------------------------------------
    //! @brief ゲームオブジェクトを破棄
    //! @param go 破棄するゲームオブジェクト
    //------------------------------------------------------------------------
    void DestroyGameObject(GameObject* go);

    //------------------------------------------------------------------------
    //! @brief 名前でゲームオブジェクトを検索
    //! @param name 検索する名前
    //! @return 見つかったゲームオブジェクト（見つからない場合はnullptr）
    //------------------------------------------------------------------------
    [[nodiscard]] GameObject* FindGameObject(const std::string& name);

    //========================================================================
    // ユーティリティ
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 全アクターとコンポーネントをクリア
    //------------------------------------------------------------------------
    void Clear();

    //------------------------------------------------------------------------
    //! @brief Systemもクリア
    //------------------------------------------------------------------------
    void ClearAll();

private:
    //------------------------------------------------------------------------
    //! @brief 型に対応するストレージを取得または作成
    //------------------------------------------------------------------------
    template<typename T>
    ComponentStorage<T>& GetOrCreateStorage() {
        std::type_index typeIndex(typeid(T));
        auto it = storages_.find(typeIndex);
        if (it == storages_.end()) {
            auto storage = std::make_unique<ComponentStorage<T>>();
            auto* ptr = storage.get();
            storages_[typeIndex] = std::move(storage);
            return *ptr;
        }
        return *static_cast<ComponentStorage<T>*>(it->second.get());
    }

    //------------------------------------------------------------------------
    //! @brief 型に対応するストレージを取得
    //------------------------------------------------------------------------
    template<typename T>
    ComponentStorage<T>* GetStorage() {
        std::type_index typeIndex(typeid(T));
        auto it = storages_.find(typeIndex);
        if (it == storages_.end()) {
            return nullptr;
        }
        return static_cast<ComponentStorage<T>*>(it->second.get());
    }

    template<typename T>
    const ComponentStorage<T>* GetStorage() const {
        std::type_index typeIndex(typeid(T));
        auto it = storages_.find(typeIndex);
        if (it == storages_.end()) {
            return nullptr;
        }
        return static_cast<const ComponentStorage<T>*>(it->second.get());
    }

private:
    //------------------------------------------------------------------------
    //! @brief システムを優先度順にソート
    //------------------------------------------------------------------------
    void SortSystems() {
        std::stable_sort(systems_.begin(), systems_.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }

    //------------------------------------------------------------------------
    //! @brief 描画システムを優先度順にソート
    //------------------------------------------------------------------------
    void SortRenderSystems() {
        std::stable_sort(renderSystems_.begin(), renderSystems_.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }

private:
    ActorManager entities_;

    //! 型消去されたComponentStorage群
    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorageBase>> storages_;

    //! System群（優先度, システム）
    std::vector<std::pair<int, std::unique_ptr<ISystem>>> systems_;
    std::vector<std::pair<int, std::unique_ptr<IRenderSystem>>> renderSystems_;

    //! OOP用GameObjectコンテナ
    std::vector<std::unique_ptr<GameObject>> gameObjects_;
};

} // namespace ECS
