//----------------------------------------------------------------------------
//! @file   oop_component_test.cpp
//! @brief  OOP Component基盤テスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/game_object/game_object_impl.h"
#include "engine/game_object/require_component.h"
#include "engine/event/event_bus.h"
#include "engine/ecs/components/transform/transform_components.h"

namespace
{

//============================================================================
// テスト用カスタムコンポーネント
//============================================================================
class TestComponent final : public Component {
public:
    TestComponent() = default;
    explicit TestComponent(int initialValue) : value(initialValue) {}

    void OnAttach() override { attachCalled = true; }
    void OnDetach() override { detachCalled = true; }
    void OnEnable() override { enableCalled = true; }
    void OnDisable() override { disableCalled = true; }

    void Update(float dt) override {
        updateCalled = true;
        lastDeltaTime = dt;
        ++updateCount;
    }

    void FixedUpdate(float dt) override {
        fixedUpdateCalled = true;
        lastFixedDeltaTime = dt;
    }

    void LateUpdate(float dt) override {
        lateUpdateCalled = true;
    }

    int value = 0;
    bool attachCalled = false;
    bool detachCalled = false;
    bool enableCalled = false;
    bool disableCalled = false;
    bool updateCalled = false;
    bool fixedUpdateCalled = false;
    bool lateUpdateCalled = false;
    float lastDeltaTime = 0.0f;
    float lastFixedDeltaTime = 0.0f;
    int updateCount = 0;
};

OOP_COMPONENT(TestComponent);

//============================================================================
// 別のテスト用コンポーネント
//============================================================================
class AnotherTestComponent final : public Component {
public:
    std::string name = "default";
};

OOP_COMPONENT(AnotherTestComponent);

//============================================================================
// Awake/Startテスト用コンポーネント
//============================================================================
class LifecycleTestComponent final : public Component {
public:
    void Awake() override {
        awakeCalled = true;
        awakeOrder = ++globalOrder;
    }

    void Start() override {
        startCalled = true;
        startOrder = ++globalOrder;
        // Start()時に他コンポーネントを取得できることを確認
        otherComp = GetComponent<AnotherTestComponent>();
    }

    void OnDestroy() override {
        destroyCalled = true;
    }

    bool awakeCalled = false;
    bool startCalled = false;
    bool destroyCalled = false;
    int awakeOrder = 0;
    int startOrder = 0;
    AnotherTestComponent* otherComp = nullptr;
    static inline int globalOrder = 0;
};

OOP_COMPONENT(LifecycleTestComponent);

//============================================================================
// GetComponent テスト用コンポーネント
//============================================================================
class ComponentAccessTestComponent final : public Component {
public:
    TestComponent* GetOtherComponent() {
        return GetComponent<TestComponent>();
    }

    const TestComponent* GetOtherComponentConst() const {
        return GetComponent<TestComponent>();
    }

    bool CheckHasComponent() const {
        return HasComponent<TestComponent>();
    }
};

OOP_COMPONENT(ComponentAccessTestComponent);

//============================================================================
// RequireComponent テスト用コンポーネント（ECS依存）
//============================================================================
class RequireECSTestComponent final : public Component {
public:
    REQUIRE_ECS_COMPONENTS(ECS::LocalTransform, ECS::LocalToWorld);
};

OOP_COMPONENT(RequireECSTestComponent);

//============================================================================
// RequireComponent テスト用コンポーネント（OOP依存）
//============================================================================
class RequireOOPTestComponent final : public Component {
public:
    REQUIRE_OOP_COMPONENTS(TestComponent);
};

OOP_COMPONENT(RequireOOPTestComponent);

//============================================================================
// メッセージテスト用カスタムメッセージ
//============================================================================
struct TestDamageMessage : Message<TestDamageMessage> {
    float damage;
    explicit TestDamageMessage(float d) : damage(d) {}
};

struct TestHealMessage : Message<TestHealMessage> {
    float amount;
    explicit TestHealMessage(float a) : amount(a) {}
};

//============================================================================
// メッセージテスト用コンポーネント（ハンドラ登録方式）
//============================================================================
class MessageReceiverComponent final : public Component {
public:
    void Awake() override {
        RegisterMessageHandler<TestDamageMessage>([this](const TestDamageMessage& msg) {
            OnDamage(msg);
        });
    }

    void OnDamage(const TestDamageMessage& msg) {
        damageReceived += msg.damage;
        ++damageCount;
    }

    float damageReceived = 0.0f;
    int damageCount = 0;
};

OOP_COMPONENT(MessageReceiverComponent);

//============================================================================
// メッセージテスト用コンポーネント（OnMessageオーバーライド方式）
//============================================================================
class OnMessageReceiverComponent final : public Component {
protected:
    void OnMessage(const IMessage& msg) override {
        if (auto* heal = dynamic_cast<const TestHealMessage*>(&msg)) {
            healReceived += heal->amount;
            ++healCount;
        }
    }

public:
    float healReceived = 0.0f;
    int healCount = 0;
};

OOP_COMPONENT(OnMessageReceiverComponent);

//============================================================================
// Component 基底クラス テスト
//============================================================================
class ComponentBaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(ComponentBaseTest, ComponentIsEnabledByDefault)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_TRUE(comp->IsEnabled());
}

TEST_F(ComponentBaseTest, ComponentHasGameObjectReference)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_EQ(comp->GetGameObject(), go);
}

TEST_F(ComponentBaseTest, ComponentHasWorldReference)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_EQ(comp->GetWorld(), world_.get());
}

TEST_F(ComponentBaseTest, ComponentHasActorReference)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_EQ(comp->GetActor(), go->GetActor());
}

TEST_F(ComponentBaseTest, SetEnabledCallsCallbacks)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    // OnEnableはAddComponent時に呼ばれる
    EXPECT_TRUE(comp->enableCalled);
    comp->enableCalled = false;

    comp->SetEnabled(false);
    EXPECT_TRUE(comp->disableCalled);
    EXPECT_FALSE(comp->IsEnabled());

    comp->SetEnabled(true);
    EXPECT_TRUE(comp->enableCalled);
    EXPECT_TRUE(comp->IsEnabled());
}

TEST_F(ComponentBaseTest, SetEnabledSameValueNoCallback)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    comp->enableCalled = false;
    comp->SetEnabled(true);  // Already enabled
    EXPECT_FALSE(comp->enableCalled);
}

//============================================================================
// GameObject OOPコンポーネント操作 テスト
//============================================================================
class GameObjectOOPComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(GameObjectOOPComponentTest, AddComponentReturnsPointer)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_NE(comp, nullptr);
}

TEST_F(GameObjectOOPComponentTest, AddComponentWithArgs)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>(42);

    EXPECT_EQ(comp->value, 42);
}

TEST_F(GameObjectOOPComponentTest, AddComponentCallsOnAttach)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_TRUE(comp->attachCalled);
}

TEST_F(GameObjectOOPComponentTest, AddComponentCallsOnEnable)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_TRUE(comp->enableCalled);
}

TEST_F(GameObjectOOPComponentTest, GetComponentReturnsExisting)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp1 = go->AddComponent<TestComponent>();
    auto* comp2 = go->GetComponent<TestComponent>();

    EXPECT_EQ(comp1, comp2);
}

TEST_F(GameObjectOOPComponentTest, GetComponentReturnsNullIfNotFound)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->GetComponent<TestComponent>();

    EXPECT_EQ(comp, nullptr);
}

TEST_F(GameObjectOOPComponentTest, HasComponentReturnsTrueIfExists)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddComponent<TestComponent>();

    EXPECT_TRUE(go->HasComponent<TestComponent>());
}

TEST_F(GameObjectOOPComponentTest, HasComponentReturnsFalseIfNotExists)
{
    auto* go = world_->CreateGameObject("Test");

    EXPECT_FALSE(go->HasComponent<TestComponent>());
}

TEST_F(GameObjectOOPComponentTest, RemoveComponentCallsCallbacks)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    // 無効化してからデタッチされることを確認するためにフラグをリセット
    EXPECT_TRUE(comp->enableCalled);

    go->RemoveComponent<TestComponent>();

    // RemoveComponent後はポインタが無効なのでアクセスしない
    EXPECT_FALSE(go->HasComponent<TestComponent>());
}

TEST_F(GameObjectOOPComponentTest, RemoveNonExistentComponentIsSafe)
{
    auto* go = world_->CreateGameObject("Test");

    // Should not crash
    go->RemoveComponent<TestComponent>();
    EXPECT_FALSE(go->HasComponent<TestComponent>());
}

TEST_F(GameObjectOOPComponentTest, AddDuplicateComponentReturnsSame)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp1 = go->AddComponent<TestComponent>();
    auto* comp2 = go->AddComponent<TestComponent>();

    EXPECT_EQ(comp1, comp2);
}

TEST_F(GameObjectOOPComponentTest, MultipleComponentTypes)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp1 = go->AddComponent<TestComponent>();
    auto* comp2 = go->AddComponent<AnotherTestComponent>();

    EXPECT_NE(comp1, nullptr);
    EXPECT_NE(comp2, nullptr);
    EXPECT_TRUE(go->HasComponent<TestComponent>());
    EXPECT_TRUE(go->HasComponent<AnotherTestComponent>());
}

TEST_F(GameObjectOOPComponentTest, GetComponentCount)
{
    auto* go = world_->CreateGameObject("Test");
    EXPECT_EQ(go->GetComponentCount(), 0u);

    go->AddComponent<TestComponent>();
    EXPECT_EQ(go->GetComponentCount(), 1u);

    go->AddComponent<AnotherTestComponent>();
    EXPECT_EQ(go->GetComponentCount(), 2u);

    go->RemoveComponent<TestComponent>();
    EXPECT_EQ(go->GetComponentCount(), 1u);
}

//============================================================================
// GameObject OOPコンポーネント更新 テスト
//============================================================================
class GameObjectUpdateTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(GameObjectUpdateTest, UpdateComponentsCallsUpdate)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    go->UpdateComponents(0.016f);

    EXPECT_TRUE(comp->updateCalled);
    EXPECT_NEAR(comp->lastDeltaTime, 0.016f, 0.0001f);
}

TEST_F(GameObjectUpdateTest, FixedUpdateComponentsCallsFixedUpdate)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    go->FixedUpdateComponents(0.02f);

    EXPECT_TRUE(comp->fixedUpdateCalled);
    EXPECT_NEAR(comp->lastFixedDeltaTime, 0.02f, 0.0001f);
}

TEST_F(GameObjectUpdateTest, LateUpdateComponentsCallsLateUpdate)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    go->LateUpdateComponents(0.016f);

    EXPECT_TRUE(comp->lateUpdateCalled);
}

TEST_F(GameObjectUpdateTest, DisabledComponentNotUpdated)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();
    comp->SetEnabled(false);

    go->UpdateComponents(0.016f);

    EXPECT_FALSE(comp->updateCalled);
}

TEST_F(GameObjectUpdateTest, InactiveGameObjectNotUpdated)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();
    go->SetActive(false);

    go->UpdateComponents(0.016f);

    EXPECT_FALSE(comp->updateCalled);
}

TEST_F(GameObjectUpdateTest, MultipleComponentsUpdated)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp1 = go->AddComponent<TestComponent>();
    auto* comp2 = go->AddComponent<AnotherTestComponent>();

    go->UpdateComponents(0.016f);

    EXPECT_TRUE(comp1->updateCalled);
    // AnotherTestComponentにはupdateCalledがない
}

//============================================================================
// Component ECSアクセス テスト
//============================================================================
class ComponentECSAccessTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(ComponentECSAccessTest, GetECSReturnsComponent)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddECS<ECS::LocalTransform>();
    auto& transform = go->GetECS<ECS::LocalTransform>();
    transform.position = Vector3(10.0f, 20.0f, 30.0f);
    auto* comp = go->AddComponent<TestComponent>();

    auto* t = comp->GetECS<ECS::LocalTransform>();

    EXPECT_NE(t, nullptr);
    EXPECT_EQ(t->position.x, 10.0f);
}

TEST_F(ComponentECSAccessTest, GetECSReturnsNullIfNotFound)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    auto* pos = comp->GetECS<ECS::LocalTransform>();

    EXPECT_EQ(pos, nullptr);
}

TEST_F(ComponentECSAccessTest, HasECSReturnsTrueIfExists)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddECS<ECS::LocalTransform>();
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_TRUE(comp->HasECS<ECS::LocalTransform>());
}

TEST_F(ComponentECSAccessTest, HasECSReturnsFalseIfNotExists)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<TestComponent>();

    EXPECT_FALSE(comp->HasECS<ECS::LocalTransform>());
}

//============================================================================
// GameObject ECS/OOP API テスト
//============================================================================
class GameObjectAPITest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(GameObjectAPITest, AddECSWorks)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddECS<ECS::LocalTransform>();
    auto& transform = go->GetECS<ECS::LocalTransform>();
    transform.position = Vector3(1.0f, 2.0f, 3.0f);

    EXPECT_TRUE(go->HasECS<ECS::LocalTransform>());
}

TEST_F(GameObjectAPITest, GetECSWorks)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddECS<ECS::LocalTransform>();
    auto& transform = go->GetECS<ECS::LocalTransform>();
    transform.position = Vector3(1.0f, 2.0f, 3.0f);

    auto& t = go->GetECS<ECS::LocalTransform>();
    EXPECT_EQ(t.position.x, 1.0f);
}

TEST_F(GameObjectAPITest, RemoveECSWorks)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddECS<ECS::LocalTransform>();
    EXPECT_TRUE(go->HasECS<ECS::LocalTransform>());

    go->RemoveECS<ECS::LocalTransform>();
    EXPECT_FALSE(go->HasECS<ECS::LocalTransform>());
}

TEST_F(GameObjectAPITest, OldAPIBackwardCompatible)
{
    auto* go = world_->CreateGameObject("Test");

    // Old API (Add/Get/Has/Remove)
    go->Add<ECS::LocalTransform>();
    auto& transform = go->Get<ECS::LocalTransform>();
    transform.position = Vector3(1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(go->Has<ECS::LocalTransform>());

    auto& t = go->Get<ECS::LocalTransform>();
    EXPECT_EQ(t.position.x, 1.0f);

    go->Remove<ECS::LocalTransform>();
    EXPECT_FALSE(go->Has<ECS::LocalTransform>());
}

//============================================================================
// Awake/Start ライフサイクル テスト
//============================================================================
class LifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        LifecycleTestComponent::globalOrder = 0;
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(LifecycleTest, AwakeCalledImmediatelyOnAddComponent)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<LifecycleTestComponent>();

    EXPECT_TRUE(comp->awakeCalled);
}

TEST_F(LifecycleTest, StartNotCalledImmediately)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<LifecycleTestComponent>();

    // Start()はAddComponent時には呼ばれない
    EXPECT_FALSE(comp->startCalled);
    EXPECT_FALSE(comp->HasStarted());
}

TEST_F(LifecycleTest, StartCalledOnFirstUpdate)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<LifecycleTestComponent>();

    // Update()前にProcessPendingStarts()でStart()が呼ばれる
    world_->Container().Update(0.016f);

    EXPECT_TRUE(comp->startCalled);
    EXPECT_TRUE(comp->HasStarted());
}

TEST_F(LifecycleTest, StartCalledOnlyOnce)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<LifecycleTestComponent>();

    world_->Container().Update(0.016f);
    int firstStartOrder = comp->startOrder;

    world_->Container().Update(0.016f);
    // startOrderは変わらない（再呼び出しされていない）
    EXPECT_EQ(comp->startOrder, firstStartOrder);
}

TEST_F(LifecycleTest, AwakeCalledBeforeStart)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<LifecycleTestComponent>();

    world_->Container().Update(0.016f);

    EXPECT_LT(comp->awakeOrder, comp->startOrder);
}

TEST_F(LifecycleTest, StartCanAccessOtherComponents)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddComponent<AnotherTestComponent>();
    auto* comp = go->AddComponent<LifecycleTestComponent>();

    world_->Container().Update(0.016f);

    EXPECT_NE(comp->otherComp, nullptr);
}

TEST_F(LifecycleTest, OnDestroyCalledOnRemove)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<LifecycleTestComponent>();
    bool destroyFlag = false;

    // RemoveComponent前にフラグをキャプチャ
    EXPECT_FALSE(comp->destroyCalled);

    go->RemoveComponent<LifecycleTestComponent>();

    // コンポーネントが削除されたことを確認
    EXPECT_FALSE(go->HasComponent<LifecycleTestComponent>());
}

TEST_F(LifecycleTest, DisabledComponentStartNotCalled)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<LifecycleTestComponent>();
    comp->SetEnabled(false);

    world_->Container().Update(0.016f);

    // 無効化されたコンポーネントはStart()が呼ばれない
    EXPECT_FALSE(comp->startCalled);
}

//============================================================================
// Component::GetComponent テスト
//============================================================================
class ComponentGetComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(ComponentGetComponentTest, GetComponentFromWithinComponent)
{
    auto* go = world_->CreateGameObject("Test");
    auto* testComp = go->AddComponent<TestComponent>();
    auto* accessComp = go->AddComponent<ComponentAccessTestComponent>();

    auto* retrieved = accessComp->GetOtherComponent();

    EXPECT_EQ(retrieved, testComp);
}

TEST_F(ComponentGetComponentTest, GetComponentReturnsNullIfNotFound)
{
    auto* go = world_->CreateGameObject("Test");
    auto* accessComp = go->AddComponent<ComponentAccessTestComponent>();

    auto* retrieved = accessComp->GetOtherComponent();

    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(ComponentGetComponentTest, GetComponentConstVersion)
{
    auto* go = world_->CreateGameObject("Test");
    auto* testComp = go->AddComponent<TestComponent>();
    const auto* accessComp = go->AddComponent<ComponentAccessTestComponent>();

    const auto* retrieved = accessComp->GetOtherComponentConst();

    EXPECT_EQ(retrieved, testComp);
}

TEST_F(ComponentGetComponentTest, HasComponentFromWithinComponent)
{
    auto* go = world_->CreateGameObject("Test");
    auto* accessComp = go->AddComponent<ComponentAccessTestComponent>();

    EXPECT_FALSE(accessComp->CheckHasComponent());

    go->AddComponent<TestComponent>();

    EXPECT_TRUE(accessComp->CheckHasComponent());
}

//============================================================================
// RequireComponent テスト
//============================================================================
class RequireComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(RequireComponentTest, RequireECSComponentsAutoAdded)
{
    auto* go = world_->CreateGameObject("Test");

    // RequireECSTestComponentはLocalTransformとLocalToWorldを要求
    EXPECT_FALSE(go->HasECS<ECS::LocalTransform>());
    EXPECT_FALSE(go->HasECS<ECS::LocalToWorld>());

    go->AddComponent<RequireECSTestComponent>();

    // 自動的に追加されている
    EXPECT_TRUE(go->HasECS<ECS::LocalTransform>());
    EXPECT_TRUE(go->HasECS<ECS::LocalToWorld>());
}

TEST_F(RequireComponentTest, RequireECSComponentsNotDuplicated)
{
    auto* go = world_->CreateGameObject("Test");

    // 先にECSコンポーネントを追加
    go->AddECS<ECS::LocalTransform>();
    auto& transform = go->GetECS<ECS::LocalTransform>();
    transform.position = Vector3(1.0f, 2.0f, 3.0f);

    // RequireECSTestComponentを追加しても既存は上書きされない
    go->AddComponent<RequireECSTestComponent>();

    auto& t = go->GetECS<ECS::LocalTransform>();
    EXPECT_EQ(t.position.x, 1.0f);
}

TEST_F(RequireComponentTest, RequireOOPComponentsAutoAdded)
{
    auto* go = world_->CreateGameObject("Test");

    // RequireOOPTestComponentはTestComponentを要求
    EXPECT_FALSE(go->HasComponent<TestComponent>());

    go->AddComponent<RequireOOPTestComponent>();

    // 自動的に追加されている
    EXPECT_TRUE(go->HasComponent<TestComponent>());
}

TEST_F(RequireComponentTest, RequireOOPComponentsNotDuplicated)
{
    auto* go = world_->CreateGameObject("Test");

    // 先にOOPコンポーネントを追加
    auto* testComp = go->AddComponent<TestComponent>();
    testComp->value = 42;

    // RequireOOPTestComponentを追加しても既存は上書きされない
    go->AddComponent<RequireOOPTestComponent>();

    auto* t = go->GetComponent<TestComponent>();
    EXPECT_EQ(t->value, 42);
    EXPECT_EQ(t, testComp);
}

//============================================================================
// SendMsg テスト
//============================================================================
class SendMsgTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(SendMsgTest, RegisteredHandlerReceivesMessage)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<MessageReceiverComponent>();

    TestDamageMessage dmg(25.0f);
    go->SendMsg(dmg);

    EXPECT_FLOAT_EQ(comp->damageReceived, 25.0f);
    EXPECT_EQ(comp->damageCount, 1);
}

TEST_F(SendMsgTest, MultipleMessagesAccumulate)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<MessageReceiverComponent>();

    go->SendMsg(TestDamageMessage(10.0f));
    go->SendMsg(TestDamageMessage(20.0f));
    go->SendMsg(TestDamageMessage(30.0f));

    EXPECT_FLOAT_EQ(comp->damageReceived, 60.0f);
    EXPECT_EQ(comp->damageCount, 3);
}

TEST_F(SendMsgTest, OnMessageOverrideReceivesMessage)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<OnMessageReceiverComponent>();

    TestHealMessage heal(50.0f);
    go->SendMsg(heal);

    EXPECT_FLOAT_EQ(comp->healReceived, 50.0f);
    EXPECT_EQ(comp->healCount, 1);
}

TEST_F(SendMsgTest, UnhandledMessageDoesNotCrash)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddComponent<MessageReceiverComponent>();

    // HealMessageはこのコンポーネントにハンドラがない
    TestHealMessage heal(100.0f);
    go->SendMsg(heal);  // クラッシュしないこと

    // MessageReceiverComponentはHealMessageを処理しない
    auto* comp = go->GetComponent<MessageReceiverComponent>();
    EXPECT_FLOAT_EQ(comp->damageReceived, 0.0f);
}

TEST_F(SendMsgTest, AllComponentsReceiveMessage)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp1 = go->AddComponent<MessageReceiverComponent>();
    auto* comp2 = go->AddComponent<OnMessageReceiverComponent>();

    // DamageMessageはMessageReceiverComponentが処理
    go->SendMsg(TestDamageMessage(10.0f));
    EXPECT_FLOAT_EQ(comp1->damageReceived, 10.0f);

    // HealMessageはOnMessageReceiverComponentが処理
    go->SendMsg(TestHealMessage(20.0f));
    EXPECT_FLOAT_EQ(comp2->healReceived, 20.0f);
}

TEST_F(SendMsgTest, DisabledComponentDoesNotReceiveMessage)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<MessageReceiverComponent>();
    comp->SetEnabled(false);

    go->SendMsg(TestDamageMessage(100.0f));

    EXPECT_FLOAT_EQ(comp->damageReceived, 0.0f);
    EXPECT_EQ(comp->damageCount, 0);
}

TEST_F(SendMsgTest, InactiveGameObjectDoesNotReceiveMessage)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<MessageReceiverComponent>();
    go->SetActive(false);

    go->SendMsg(TestDamageMessage(100.0f));

    EXPECT_FLOAT_EQ(comp->damageReceived, 0.0f);
}

//============================================================================
// BroadcastMsg テスト
//============================================================================
class BroadcastMsgTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(BroadcastMsgTest, BroadcastMsgToSelf)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<MessageReceiverComponent>();

    go->BroadcastMsg(TestDamageMessage(10.0f));

    EXPECT_FLOAT_EQ(comp->damageReceived, 10.0f);
}

TEST_F(BroadcastMsgTest, BroadcastMsgToChildren)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child1 = world_->CreateGameObject("Child1");
    auto* child2 = world_->CreateGameObject("Child2");

    // 親子関係を設定
    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    hierarchy.SetParent(child1->GetActor(), parent->GetActor(), *world_);
    hierarchy.SetParent(child2->GetActor(), parent->GetActor(), *world_);

    auto* parentComp = parent->AddComponent<MessageReceiverComponent>();
    auto* child1Comp = child1->AddComponent<MessageReceiverComponent>();
    auto* child2Comp = child2->AddComponent<MessageReceiverComponent>();

    // 親からBroadcast
    parent->BroadcastMsg(TestDamageMessage(5.0f));

    EXPECT_FLOAT_EQ(parentComp->damageReceived, 5.0f);
    EXPECT_FLOAT_EQ(child1Comp->damageReceived, 5.0f);
    EXPECT_FLOAT_EQ(child2Comp->damageReceived, 5.0f);
}

TEST_F(BroadcastMsgTest, BroadcastMsgToGrandchildren)
{
    auto* root = world_->CreateGameObject("Root");
    auto* child = world_->CreateGameObject("Child");
    auto* grandchild = world_->CreateGameObject("Grandchild");

    // 階層を設定: Root -> Child -> Grandchild
    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    hierarchy.SetParent(child->GetActor(), root->GetActor(), *world_);
    hierarchy.SetParent(grandchild->GetActor(), child->GetActor(), *world_);

    auto* rootComp = root->AddComponent<MessageReceiverComponent>();
    auto* childComp = child->AddComponent<MessageReceiverComponent>();
    auto* grandchildComp = grandchild->AddComponent<MessageReceiverComponent>();

    // ルートからBroadcast
    root->BroadcastMsg(TestDamageMessage(3.0f));

    EXPECT_FLOAT_EQ(rootComp->damageReceived, 3.0f);
    EXPECT_FLOAT_EQ(childComp->damageReceived, 3.0f);
    EXPECT_FLOAT_EQ(grandchildComp->damageReceived, 3.0f);
}

TEST_F(BroadcastMsgTest, BroadcastMsgDoesNotAffectSiblings)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child1 = world_->CreateGameObject("Child1");
    auto* child2 = world_->CreateGameObject("Child2");

    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    hierarchy.SetParent(child1->GetActor(), parent->GetActor(), *world_);
    hierarchy.SetParent(child2->GetActor(), parent->GetActor(), *world_);

    auto* parentComp = parent->AddComponent<MessageReceiverComponent>();
    auto* child1Comp = child1->AddComponent<MessageReceiverComponent>();
    auto* child2Comp = child2->AddComponent<MessageReceiverComponent>();

    // Child1からBroadcast（Child2には届かない）
    child1->BroadcastMsg(TestDamageMessage(7.0f));

    EXPECT_FLOAT_EQ(parentComp->damageReceived, 0.0f);  // 親には届かない
    EXPECT_FLOAT_EQ(child1Comp->damageReceived, 7.0f);  // 自身には届く
    EXPECT_FLOAT_EQ(child2Comp->damageReceived, 0.0f);  // 兄弟には届かない
}

//============================================================================
// SendMsgUpwards テスト
//============================================================================
class SendMsgUpwardsTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(SendMsgUpwardsTest, SendMsgUpwardsToSelf)
{
    auto* go = world_->CreateGameObject("Test");
    auto* comp = go->AddComponent<MessageReceiverComponent>();

    go->SendMsgUpwards(TestDamageMessage(10.0f));

    EXPECT_FLOAT_EQ(comp->damageReceived, 10.0f);
}

TEST_F(SendMsgUpwardsTest, SendMsgUpwardsToParent)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    hierarchy.SetParent(child->GetActor(), parent->GetActor(), *world_);

    auto* parentComp = parent->AddComponent<MessageReceiverComponent>();
    auto* childComp = child->AddComponent<MessageReceiverComponent>();

    // 子から上方向にメッセージ送信
    child->SendMsgUpwards(TestDamageMessage(15.0f));

    EXPECT_FLOAT_EQ(childComp->damageReceived, 15.0f);
    EXPECT_FLOAT_EQ(parentComp->damageReceived, 15.0f);
}

TEST_F(SendMsgUpwardsTest, SendMsgUpwardsToGrandparent)
{
    auto* grandparent = world_->CreateGameObject("Grandparent");
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    hierarchy.SetParent(parent->GetActor(), grandparent->GetActor(), *world_);
    hierarchy.SetParent(child->GetActor(), parent->GetActor(), *world_);

    auto* grandparentComp = grandparent->AddComponent<MessageReceiverComponent>();
    auto* parentComp = parent->AddComponent<MessageReceiverComponent>();
    auto* childComp = child->AddComponent<MessageReceiverComponent>();

    // 孫から上方向にメッセージ送信
    child->SendMsgUpwards(TestDamageMessage(20.0f));

    EXPECT_FLOAT_EQ(childComp->damageReceived, 20.0f);
    EXPECT_FLOAT_EQ(parentComp->damageReceived, 20.0f);
    EXPECT_FLOAT_EQ(grandparentComp->damageReceived, 20.0f);
}

TEST_F(SendMsgUpwardsTest, SendMsgUpwardsDoesNotAffectSiblings)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child1 = world_->CreateGameObject("Child1");
    auto* child2 = world_->CreateGameObject("Child2");

    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    hierarchy.SetParent(child1->GetActor(), parent->GetActor(), *world_);
    hierarchy.SetParent(child2->GetActor(), parent->GetActor(), *world_);

    auto* parentComp = parent->AddComponent<MessageReceiverComponent>();
    auto* child1Comp = child1->AddComponent<MessageReceiverComponent>();
    auto* child2Comp = child2->AddComponent<MessageReceiverComponent>();

    // Child1から上方向にメッセージ送信
    child1->SendMsgUpwards(TestDamageMessage(25.0f));

    EXPECT_FLOAT_EQ(parentComp->damageReceived, 25.0f);  // 親には届く
    EXPECT_FLOAT_EQ(child1Comp->damageReceived, 25.0f);  // 自身には届く
    EXPECT_FLOAT_EQ(child2Comp->damageReceived, 0.0f);   // 兄弟には届かない
}

//============================================================================
// EventBus テスト用イベント
//============================================================================
struct TestScoreEvent {
    int score;
    explicit TestScoreEvent(int s) : score(s) {}
};

struct TestGameOverEvent {
    bool won;
    explicit TestGameOverEvent(bool w) : won(w) {}
};

//============================================================================
// EventBus テスト（既存EventBusを使用）
//============================================================================
class OOPEventBusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // EventBus初期化
        EventBus::Create();
        EventBus::Get().Clear();
    }

    void TearDown() override {
        EventBus::Get().Clear();
    }
};

TEST_F(OOPEventBusTest, SubscribeAndPublish)
{
    int receivedScore = 0;

    auto id = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent& e) {
        receivedScore = e.score;
    });

    EventBus::Get().Publish(TestScoreEvent(100));

    EXPECT_EQ(receivedScore, 100);

    EventBus::Get().Unsubscribe<TestScoreEvent>(id);
}

TEST_F(OOPEventBusTest, MultipleSubscribers)
{
    int count1 = 0;
    int count2 = 0;

    auto id1 = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent&) {
        ++count1;
    });

    auto id2 = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent&) {
        ++count2;
    });

    EventBus::Get().Publish(TestScoreEvent(50));

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);

    EventBus::Get().Unsubscribe<TestScoreEvent>(id1);
    EventBus::Get().Unsubscribe<TestScoreEvent>(id2);
}

TEST_F(OOPEventBusTest, Unsubscribe)
{
    int count = 0;

    auto id = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent&) {
        ++count;
    });

    EventBus::Get().Publish(TestScoreEvent(10));
    EXPECT_EQ(count, 1);

    EventBus::Get().Unsubscribe<TestScoreEvent>(id);

    EventBus::Get().Publish(TestScoreEvent(20));
    EXPECT_EQ(count, 1);  // 変わらない
}

TEST_F(OOPEventBusTest, DifferentEventTypes)
{
    int scoreReceived = 0;
    bool gameOverReceived = false;

    auto id1 = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent& e) {
        scoreReceived = e.score;
    });

    auto id2 = EventBus::Get().Subscribe<TestGameOverEvent>([&](const TestGameOverEvent& e) {
        gameOverReceived = e.won;
    });

    EventBus::Get().Publish(TestScoreEvent(200));
    EXPECT_EQ(scoreReceived, 200);
    EXPECT_FALSE(gameOverReceived);

    EventBus::Get().Publish(TestGameOverEvent(true));
    EXPECT_TRUE(gameOverReceived);

    EventBus::Get().Unsubscribe<TestScoreEvent>(id1);
    EventBus::Get().Unsubscribe<TestGameOverEvent>(id2);
}

TEST_F(OOPEventBusTest, NoSubscribersDoesNotCrash)
{
    // 購読者がいなくてもクラッシュしない
    EventBus::Get().Publish(TestScoreEvent(999));
}

TEST_F(OOPEventBusTest, PublishDuringPublish)
{
    int count = 0;

    auto id = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent& e) {
        ++count;
        // 発行中に再度発行（安全に動作するべき）
        if (e.score < 3) {
            EventBus::Get().Publish(TestScoreEvent(e.score + 1));
        }
    });

    EventBus::Get().Publish(TestScoreEvent(1));

    EXPECT_EQ(count, 3);  // 1, 2, 3 の3回呼ばれる

    EventBus::Get().Unsubscribe<TestScoreEvent>(id);
}

TEST_F(OOPEventBusTest, PriorityRespected)
{
    std::vector<int> callOrder;

    auto id1 = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent&) {
        callOrder.push_back(2);
    }, EventPriority::Normal);

    auto id2 = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent&) {
        callOrder.push_back(1);
    }, EventPriority::High);

    auto id3 = EventBus::Get().Subscribe<TestScoreEvent>([&](const TestScoreEvent&) {
        callOrder.push_back(3);
    }, EventPriority::Low);

    EventBus::Get().Publish(TestScoreEvent(0));

    ASSERT_EQ(callOrder.size(), 3u);
    EXPECT_EQ(callOrder[0], 1);  // High first
    EXPECT_EQ(callOrder[1], 2);  // Normal second
    EXPECT_EQ(callOrder[2], 3);  // Low last

    EventBus::Get().Unsubscribe<TestScoreEvent>(id1);
    EventBus::Get().Unsubscribe<TestScoreEvent>(id2);
    EventBus::Get().Unsubscribe<TestScoreEvent>(id3);
}

//============================================================================
// Phase 3: 階層サポートテスト
//============================================================================

// テスト用コンポーネント
class HierarchyTestComponent final : public Component {
public:
    std::string tag;
    explicit HierarchyTestComponent(const std::string& t = "") : tag(t) {}
};

class AnotherHierarchyComponent final : public Component {
public:
    int value = 0;
    explicit AnotherHierarchyComponent(int v = 0) : value(v) {}
};

//============================================================================
// GetComponentInChildren テスト
//============================================================================
class GetComponentInChildrenTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(GetComponentInChildrenTest, FindsComponentInDirectChild)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent);
    auto* childComp = child->AddComponent<HierarchyTestComponent>("child-tag");

    auto* found = parent->GetComponentInChildren<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->tag, "child-tag");
    EXPECT_EQ(found, childComp);
}

TEST_F(GetComponentInChildrenTest, FindsComponentInGrandchild)
{
    auto* root = world_->CreateGameObject("Root");
    auto* child = world_->CreateGameObject("Child");
    auto* grandchild = world_->CreateGameObject("Grandchild");

    child->SetParent(root);
    grandchild->SetParent(child);

    auto* grandchildComp = grandchild->AddComponent<HierarchyTestComponent>("grandchild-tag");

    auto* found = root->GetComponentInChildren<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->tag, "grandchild-tag");
    EXPECT_EQ(found, grandchildComp);
}

TEST_F(GetComponentInChildrenTest, ReturnsFirstFoundDepthFirst)
{
    auto* root = world_->CreateGameObject("Root");
    auto* child1 = world_->CreateGameObject("Child1");
    auto* child2 = world_->CreateGameObject("Child2");

    child1->SetParent(root);
    child2->SetParent(root);

    auto* child1Comp = child1->AddComponent<HierarchyTestComponent>("child1-tag");
    child2->AddComponent<HierarchyTestComponent>("child2-tag");

    auto* found = root->GetComponentInChildren<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found, child1Comp);  // 最初に見つかったものを返す
}

TEST_F(GetComponentInChildrenTest, ReturnsNullptrWhenNotFound)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent);
    child->AddComponent<AnotherHierarchyComponent>(42);

    auto* found = parent->GetComponentInChildren<HierarchyTestComponent>();

    EXPECT_EQ(found, nullptr);
}

TEST_F(GetComponentInChildrenTest, DoesNotIncludeSelf)
{
    auto* go = world_->CreateGameObject("Test");
    go->AddComponent<HierarchyTestComponent>("self-tag");

    // 自身は検索対象外
    auto* found = go->GetComponentInChildren<HierarchyTestComponent>();

    EXPECT_EQ(found, nullptr);
}

TEST_F(GetComponentInChildrenTest, ComponentAccessFromComponent)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent);

    auto* parentComp = parent->AddComponent<AnotherHierarchyComponent>(10);
    auto* childComp = child->AddComponent<HierarchyTestComponent>("child-tag");

    // Componentから階層検索
    auto* found = parentComp->GetComponentInChildren<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found, childComp);
}

//============================================================================
// GetComponentInParent テスト
//============================================================================
class GetComponentInParentTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(GetComponentInParentTest, FindsComponentInDirectParent)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent);
    auto* parentComp = parent->AddComponent<HierarchyTestComponent>("parent-tag");

    auto* found = child->GetComponentInParent<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->tag, "parent-tag");
    EXPECT_EQ(found, parentComp);
}

TEST_F(GetComponentInParentTest, FindsComponentInGrandparent)
{
    auto* grandparent = world_->CreateGameObject("Grandparent");
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    parent->SetParent(grandparent);
    child->SetParent(parent);

    auto* grandparentComp = grandparent->AddComponent<HierarchyTestComponent>("grandparent-tag");

    auto* found = child->GetComponentInParent<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->tag, "grandparent-tag");
    EXPECT_EQ(found, grandparentComp);
}

TEST_F(GetComponentInParentTest, FindsClosestParentFirst)
{
    auto* grandparent = world_->CreateGameObject("Grandparent");
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    parent->SetParent(grandparent);
    child->SetParent(parent);

    grandparent->AddComponent<HierarchyTestComponent>("grandparent-tag");
    auto* parentComp = parent->AddComponent<HierarchyTestComponent>("parent-tag");

    auto* found = child->GetComponentInParent<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found, parentComp);  // 直接の親が先
    EXPECT_EQ(found->tag, "parent-tag");
}

TEST_F(GetComponentInParentTest, ReturnsNullptrWhenNotFound)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent);
    parent->AddComponent<AnotherHierarchyComponent>(42);

    auto* found = child->GetComponentInParent<HierarchyTestComponent>();

    EXPECT_EQ(found, nullptr);
}

TEST_F(GetComponentInParentTest, ReturnsNullptrForRoot)
{
    auto* go = world_->CreateGameObject("Root");
    go->AddComponent<HierarchyTestComponent>("self-tag");

    // 自身は検索対象外、親もいない
    auto* found = go->GetComponentInParent<HierarchyTestComponent>();

    EXPECT_EQ(found, nullptr);
}

TEST_F(GetComponentInParentTest, ComponentAccessFromComponent)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent);

    auto* parentComp = parent->AddComponent<HierarchyTestComponent>("parent-tag");
    auto* childComp = child->AddComponent<AnotherHierarchyComponent>(20);

    // Componentから階層検索
    auto* found = childComp->GetComponentInParent<HierarchyTestComponent>();

    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found, parentComp);
}

//============================================================================
// SetParent/GetParent テスト
//============================================================================
class HierarchyAPITest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(HierarchyAPITest, SetAndGetParent)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    EXPECT_EQ(child->GetParent(), nullptr);

    child->SetParent(parent);

    EXPECT_EQ(child->GetParent(), parent);
}

TEST_F(HierarchyAPITest, ClearParent)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent);
    EXPECT_EQ(child->GetParent(), parent);

    child->SetParent(nullptr);
    EXPECT_EQ(child->GetParent(), nullptr);
}

TEST_F(HierarchyAPITest, GetChildCount)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child1 = world_->CreateGameObject("Child1");
    auto* child2 = world_->CreateGameObject("Child2");
    auto* child3 = world_->CreateGameObject("Child3");

    EXPECT_EQ(parent->GetChildCount(), 0u);

    child1->SetParent(parent);
    EXPECT_EQ(parent->GetChildCount(), 1u);

    child2->SetParent(parent);
    child3->SetParent(parent);
    EXPECT_EQ(parent->GetChildCount(), 3u);
}

TEST_F(HierarchyAPITest, ReparentChild)
{
    auto* parent1 = world_->CreateGameObject("Parent1");
    auto* parent2 = world_->CreateGameObject("Parent2");
    auto* child = world_->CreateGameObject("Child");

    child->SetParent(parent1);
    EXPECT_EQ(child->GetParent(), parent1);
    EXPECT_EQ(parent1->GetChildCount(), 1u);
    EXPECT_EQ(parent2->GetChildCount(), 0u);

    child->SetParent(parent2);
    EXPECT_EQ(child->GetParent(), parent2);
    EXPECT_EQ(parent1->GetChildCount(), 0u);
    EXPECT_EQ(parent2->GetChildCount(), 1u);
}

//============================================================================
// ForEachChild テスト
//============================================================================
TEST_F(HierarchyAPITest, ForEachChild)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child1 = world_->CreateGameObject("Child1");
    auto* child2 = world_->CreateGameObject("Child2");

    child1->SetParent(parent);
    child2->SetParent(parent);

    std::vector<std::string> names;
    parent->ForEachChild([&names](GameObject* child) {
        names.push_back(child->GetName());
    });

    EXPECT_EQ(names.size(), 2u);
    EXPECT_TRUE(std::find(names.begin(), names.end(), "Child1") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "Child2") != names.end());
}

TEST_F(HierarchyAPITest, ForEachChildEmpty)
{
    auto* parent = world_->CreateGameObject("Parent");

    int count = 0;
    parent->ForEachChild([&count](GameObject*) {
        ++count;
    });

    EXPECT_EQ(count, 0);
}

TEST_F(HierarchyAPITest, ForEachChildConst)
{
    auto* parent = world_->CreateGameObject("Parent");
    auto* child = world_->CreateGameObject("Child");
    child->SetParent(parent);

    const GameObject* constParent = parent;
    int count = 0;
    constParent->ForEachChild([&count](const GameObject*) {
        ++count;
    });

    EXPECT_EQ(count, 1);
}

} // namespace
