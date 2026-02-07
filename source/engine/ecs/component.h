//----------------------------------------------------------------------------
//! @file   component.h
//! @brief  OOP Component - ロジック用コンポーネント基底クラス
//----------------------------------------------------------------------------
#pragma once


#include "actor.h"
#include "engine/game_object/message.h"
#include <typeindex>

// 前方宣言
class GameObject;

namespace ECS {
    class World;
}

//============================================================================
//! @brief OOPコンポーネント基底クラス
//!
//! 複雑なロジックを実装するコンポーネントの基底。
//! ECSデータ（IComponentData）とは異なり、仮想関数によるライフサイクルを持つ。
//!
//! @note データはECS側（PositionData, RotationData等）に持たせ、
//!       このクラスはロジック（更新処理、イベントハンドリング）のみを担当。
//!
//! ライフサイクル（Unity互換）:
//! 1. Awake()      - AddComponent時に即時呼び出し
//! 2. OnEnable()   - 有効化時
//! 3. Start()      - 最初のUpdate前に1回だけ呼び出し
//! 4. Update()     - 毎フレーム
//! 5. LateUpdate() - 全Update後
//! 6. OnDisable()  - 無効化時
//! 7. OnDestroy()  - RemoveComponent/Destroy時
//!
//! @code
//! class PlayerController final : public Component {
//! public:
//!     void Awake() override {
//!         // 即時初期化（他コンポーネント参照は避ける）
//!     }
//!
//!     void Start() override {
//!         // 他コンポーネントの参照取得など
//!         otherComp_ = GetComponent<OtherComponent>();
//!     }
//!
//!     void Update(float dt) override {
//!         auto* pos = GetECS<PositionData>();
//!         if (pos) {
//!             pos->value += velocity_ * dt;
//!         }
//!     }
//!
//! private:
//!     OtherComponent* otherComp_ = nullptr;
//!     Vector3 velocity_;
//! };
//! @endcode
//============================================================================
class Component {
public:
    Component() = default;
    virtual ~Component() = default;

    // コピー禁止
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;

    //========================================================================
    // ライフサイクルコールバック（Unity互換）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief AddComponent時に即時呼び出される
    //!
    //! 自身の初期化を行う。他のコンポーネントへの参照はStart()で行うこと。
    //------------------------------------------------------------------------
    virtual void Awake() {}

    //------------------------------------------------------------------------
    //! @brief 最初のUpdate前に1回だけ呼び出される
    //!
    //! 他のコンポーネントへの参照取得など、全オブジェクト初期化後の処理に使用。
    //------------------------------------------------------------------------
    virtual void Start() {}

    //------------------------------------------------------------------------
    //! @brief コンポーネントが有効化された時に呼ばれる
    //------------------------------------------------------------------------
    virtual void OnEnable() {}

    //------------------------------------------------------------------------
    //! @brief コンポーネントが無効化された時に呼ばれる
    //------------------------------------------------------------------------
    virtual void OnDisable() {}

    //------------------------------------------------------------------------
    //! @brief RemoveComponent/Destroy時に呼ばれる
    //------------------------------------------------------------------------
    virtual void OnDestroy() {}

    //------------------------------------------------------------------------
    //! @brief コンポーネントがGameObjectにアタッチされた時に呼ばれる
    //! @deprecated Awake()を使用してください
    //------------------------------------------------------------------------
    virtual void OnAttach() {}

    //------------------------------------------------------------------------
    //! @brief コンポーネントがGameObjectからデタッチされる時に呼ばれる
    //! @deprecated OnDestroy()を使用してください
    //------------------------------------------------------------------------
    virtual void OnDetach() {}

    //========================================================================
    // 更新コールバック
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 毎フレーム更新
    //! @param dt デルタタイム（秒）
    //------------------------------------------------------------------------
    virtual void Update([[maybe_unused]] float dt) {}

    //------------------------------------------------------------------------
    //! @brief 固定タイムステップ更新（物理演算用）
    //! @param dt 固定デルタタイム（通常1/60秒）
    //------------------------------------------------------------------------
    virtual void FixedUpdate([[maybe_unused]] float dt) {}

    //------------------------------------------------------------------------
    //! @brief 全てのUpdate後に呼ばれる
    //! @param dt デルタタイム（秒）
    //------------------------------------------------------------------------
    virtual void LateUpdate([[maybe_unused]] float dt) {}

    //========================================================================
    // メッセージ受信
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief メッセージを受信
    //! @param msg 受信するメッセージ
    //! @return 処理された場合はtrue
    //!
    //! メッセージは以下の順序で処理される:
    //! 1. RegisterMessageHandler()で登録されたハンドラ
    //! 2. OnMessage()仮想関数
    //!
    //! @code
    //! // 方法1: ハンドラ登録（Awake()内で）
    //! void Awake() override {
    //!     RegisterMessageHandler<DamageMessage>([this](const DamageMessage& msg) {
    //!         health_ -= msg.amount;
    //!     });
    //! }
    //!
    //! // 方法2: OnMessage()オーバーライド
    //! void OnMessage(const IMessage& msg) override {
    //!     if (auto* dmg = dynamic_cast<const DamageMessage*>(&msg)) {
    //!         health_ -= dmg->amount;
    //!     }
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    bool ReceiveMessage(const IMessage& msg) {
        if (!enabled_) return false;

        // 登録済みハンドラを呼び出し
        if (messageHandlers_.Handle(msg)) {
            return true;
        }

        // 仮想関数を呼び出し
        OnMessage(msg);
        return true;
    }

    //------------------------------------------------------------------------
    //! @brief メッセージハンドラを登録
    //! @tparam T メッセージ型
    //! @param handler ハンドラ関数
    //!
    //! @code
    //! void Awake() override {
    //!     RegisterMessageHandler<DamageMessage>([this](const DamageMessage& msg) {
    //!         health_ -= msg.amount;
    //!     });
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    void RegisterMessageHandler(std::function<void(const T&)> handler) {
        messageHandlers_.Register<T>(std::move(handler));
    }

protected:
    //------------------------------------------------------------------------
    //! @brief メッセージ受信時の仮想コールバック
    //! @param msg 受信したメッセージ
    //!
    //! 派生クラスでオーバーライドしてメッセージを処理する。
    //! RegisterMessageHandler()で処理されなかったメッセージがここに来る。
    //------------------------------------------------------------------------
    virtual void OnMessage([[maybe_unused]] const IMessage& msg) {}

public:
    //========================================================================
    // OOPコンポーネントアクセス（同一GameObject内）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 同一GameObject内のOOPコンポーネントを取得
    //! @tparam T OOPコンポーネント型（Componentを継承）
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //!
    //! @code
    //! void PlayerController::Start() {
    //!     rigidbody_ = GetComponent<Rigidbody>();
    //!     animator_ = GetComponent<Animator>();
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponent();

    template<typename T>
    [[nodiscard]] const T* GetComponent() const;

    //------------------------------------------------------------------------
    //! @brief 同一GameObject内にOOPコンポーネントがあるか確認
    //! @tparam T OOPコンポーネント型
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasComponent() const;

    //========================================================================
    // 階層コンポーネントアクセス
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 子階層からコンポーネントを検索
    //! @tparam T OOPコンポーネント型
    //! @return 最初に見つかったコンポーネント（存在しない場合はnullptr）
    //!
    //! @code
    //! void Start() override {
    //!     weapon_ = GetComponentInChildren<Weapon>();
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponentInChildren();

    template<typename T>
    [[nodiscard]] const T* GetComponentInChildren() const;

    //------------------------------------------------------------------------
    //! @brief 親階層からコンポーネントを検索
    //! @tparam T OOPコンポーネント型
    //! @return 最初に見つかったコンポーネント（存在しない場合はnullptr）
    //!
    //! @code
    //! void Start() override {
    //!     manager_ = GetComponentInParent<GameManager>();
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponentInParent();

    template<typename T>
    [[nodiscard]] const T* GetComponentInParent() const;

    //========================================================================
    // ECSデータアクセス
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief ECSコンポーネントを取得
    //! @tparam T ECSコンポーネント型（IComponentDataを継承）
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //!
    //! @code
    //! void PlayerController::Update(float dt) {
    //!     auto* pos = GetECS<PositionData>();
    //!     auto* rot = GetECS<RotationData>();
    //!     if (pos && rot) {
    //!         // 移動処理
    //!     }
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetECS();

    template<typename T>
    [[nodiscard]] const T* GetECS() const;

    //------------------------------------------------------------------------
    //! @brief ECSコンポーネントを所持しているか確認
    //! @tparam T ECSコンポーネント型
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasECS() const;

    //========================================================================
    // アクセサ
    //========================================================================

    //! @brief 所有GameObjectを取得
    [[nodiscard]] GameObject* GetGameObject() const noexcept { return gameObject_; }

    //! @brief ECS Actorを取得
    [[nodiscard]] ECS::Actor GetActor() const noexcept { return actor_; }

    //! @brief ECS Worldを取得
    [[nodiscard]] ECS::World* GetWorld() const noexcept { return world_; }

    //! @brief 有効状態を取得
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

    //! @brief 有効状態を設定
    void SetEnabled(bool enabled);

    //! @brief Start()が呼ばれたかどうか
    [[nodiscard]] bool HasStarted() const noexcept { return started_; }

    //! @brief 型IDを取得
    [[nodiscard]] std::type_index GetTypeId() const noexcept { return typeId_; }

protected:
    //------------------------------------------------------------------------
    //! @brief 内部初期化（GameObjectから呼ばれる）
    //! @param gameObject 所有GameObject
    //! @param actor ECS Actor
    //! @param world ECS World
    //! @param typeId コンポーネントの型ID
    //------------------------------------------------------------------------
    void Initialize(GameObject* gameObject, ECS::Actor actor, ECS::World* world, std::type_index typeId) {
        gameObject_ = gameObject;
        actor_ = actor;
        world_ = world;
        typeId_ = typeId;
    }

public:
    //! @brief Start()を呼び出してstarted_をtrueにする（GameObjectContainerから呼ばれる）
    //! @note 内部使用専用。ユーザーコードから直接呼び出さないこと。
    void InvokeStart() {
        if (!started_ && enabled_) {
            Start();
            started_ = true;
        }
    }

private:
    friend class GameObject;

    GameObject* gameObject_ = nullptr;           //!< 所有GameObject
    ECS::Actor actor_;                           //!< ECS Actor
    ECS::World* world_ = nullptr;                //!< ECS World
    std::type_index typeId_ = typeid(void);      //!< コンポーネントの型ID
    bool enabled_ = true;                        //!< 有効状態
    bool started_ = false;                       //!< Start()が呼ばれたかどうか
    MessageHandlerMap messageHandlers_;          //!< メッセージハンドラマップ
};

//============================================================================
//! @brief OOPコンポーネント検証マクロ
//!
//! コンポーネント型がOOPコンポーネントの要件を満たしているか検証。
//!
//! @param Type 検証するコンポーネント型
//!
//! @code
//! class PlayerController final : public Component { ... };
//! OOP_COMPONENT(PlayerController);
//! @endcode
//============================================================================
#define OOP_COMPONENT(Type)                                                     \
    static_assert(std::is_base_of_v<Component, Type>,                          \
        #Type " must inherit from Component");                                  \
    static_assert(std::is_final_v<Type>,                                       \
        #Type " should be marked as final for performance")
