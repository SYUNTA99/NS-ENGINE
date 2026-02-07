//----------------------------------------------------------------------------
//! @file   message.h
//! @brief  メッセージシステム - コンポーネント間通信
//----------------------------------------------------------------------------
#pragma once


#include <typeindex>
#include <functional>
#include <unordered_map>
#include <vector>
#include <any>

//============================================================================
//! @brief メッセージ基底クラス
//!
//! 全てのメッセージ型はこの構造体を継承する。
//! CRTP（Curiously Recurring Template Pattern）で型IDを自動生成。
//!
//! @code
//! struct DamageMessage : Message<DamageMessage> {
//!     float amount;
//!     explicit DamageMessage(float a) : amount(a) {}
//! };
//! @endcode
//============================================================================
struct IMessage {
    virtual ~IMessage() = default;
    [[nodiscard]] virtual std::type_index GetTypeId() const noexcept = 0;
};

//============================================================================
//! @brief 型付きメッセージ基底（CRTP）
//!
//! @tparam Derived 派生メッセージ型
//============================================================================
template<typename Derived>
struct Message : IMessage {
    [[nodiscard]] std::type_index GetTypeId() const noexcept override {
        return typeid(Derived);
    }

    //! 型IDを静的に取得
    [[nodiscard]] static std::type_index StaticTypeId() noexcept {
        return typeid(Derived);
    }
};

//============================================================================
//! @brief メッセージハンドラ登録用マクロ
//!
//! Component内でメッセージハンドラを簡単に登録するためのマクロ。
//!
//! @code
//! class PlayerController : public Component {
//!     void Awake() override {
//!         REGISTER_MESSAGE_HANDLER(DamageMessage, OnDamage);
//!     }
//!
//!     void OnDamage(const DamageMessage& msg) {
//!         health_ -= msg.amount;
//!     }
//! };
//! @endcode
//============================================================================
#define REGISTER_MESSAGE_HANDLER(MsgType, Handler)                              \
    RegisterMessageHandler<MsgType>([this](const MsgType& msg) { Handler(msg); })

//============================================================================
//! @brief メッセージレシーバーインターフェース
//!
//! メッセージを受信できるオブジェクトが実装するインターフェース。
//============================================================================
class IMessageReceiver {
public:
    virtual ~IMessageReceiver() = default;

    //------------------------------------------------------------------------
    //! @brief メッセージを受信
    //! @param msg 受信するメッセージ
    //! @return メッセージが処理された場合はtrue
    //------------------------------------------------------------------------
    virtual bool ReceiveMessage(const IMessage& msg) = 0;
};

//============================================================================
//! @brief メッセージハンドラマップ
//!
//! 型IDとハンドラ関数のマッピングを保持。
//! Component基底クラスで使用。
//============================================================================
class MessageHandlerMap {
public:
    //------------------------------------------------------------------------
    //! @brief ハンドラを登録
    //! @tparam T メッセージ型
    //! @param handler ハンドラ関数
    //------------------------------------------------------------------------
    template<typename T>
    void Register(std::function<void(const T&)> handler) {
        handlers_[typeid(T)] = [handler = std::move(handler)](const IMessage& msg) {
            handler(static_cast<const T&>(msg));
        };
    }

    //------------------------------------------------------------------------
    //! @brief メッセージを処理
    //! @param msg 処理するメッセージ
    //! @return 処理された場合はtrue
    //------------------------------------------------------------------------
    bool Handle(const IMessage& msg) const {
        auto it = handlers_.find(msg.GetTypeId());
        if (it != handlers_.end()) {
            it->second(msg);
            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------
    //! @brief ハンドラをクリア
    //------------------------------------------------------------------------
    void Clear() {
        handlers_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief 登録済みハンドラ数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Count() const noexcept {
        return handlers_.size();
    }

private:
    std::unordered_map<std::type_index, std::function<void(const IMessage&)>> handlers_;
};

//============================================================================
// 標準メッセージ型
//============================================================================

//! @brief 有効化メッセージ
struct EnableMessage : Message<EnableMessage> {};

//! @brief 無効化メッセージ
struct DisableMessage : Message<DisableMessage> {};

//! @brief 破棄メッセージ
struct DestroyMessage : Message<DestroyMessage> {};

//! @brief 衝突開始メッセージ
struct CollisionEnterMessage : Message<CollisionEnterMessage> {
    class GameObject* other = nullptr;
    explicit CollisionEnterMessage(GameObject* o) : other(o) {}
};

//! @brief 衝突中メッセージ
struct CollisionStayMessage : Message<CollisionStayMessage> {
    class GameObject* other = nullptr;
    explicit CollisionStayMessage(GameObject* o) : other(o) {}
};

//! @brief 衝突終了メッセージ
struct CollisionExitMessage : Message<CollisionExitMessage> {
    class GameObject* other = nullptr;
    explicit CollisionExitMessage(GameObject* o) : other(o) {}
};

//! @brief トリガー開始メッセージ
struct TriggerEnterMessage : Message<TriggerEnterMessage> {
    class GameObject* other = nullptr;
    explicit TriggerEnterMessage(GameObject* o) : other(o) {}
};

//! @brief トリガー終了メッセージ
struct TriggerExitMessage : Message<TriggerExitMessage> {
    class GameObject* other = nullptr;
    explicit TriggerExitMessage(GameObject* o) : other(o) {}
};

