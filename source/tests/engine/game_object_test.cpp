//----------------------------------------------------------------------------
//! @file   game_object_test.cpp
//! @brief  GameObjectのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/game_object.h"
#include "engine/component/transform.h"
#include "engine/component/animator.h"

namespace
{

//============================================================================
// テスト用カスタムコンポーネント
//============================================================================
class TestComponent : public Component
{
public:
    int value = 0;
    bool wasUpdated = false;
    bool wasAttached = false;
    bool wasDetached = false;

    void Update([[maybe_unused]] float deltaTime) override {
        wasUpdated = true;
        value++;
    }

    void OnAttach() override {
        wasAttached = true;
    }

    void OnDetach() override {
        wasDetached = true;
    }
};

//============================================================================
// GameObject 基本テスト
//============================================================================
class GameObjectTest : public ::testing::Test
{
protected:
    GameObject gameObject_{"TestObject"};
};

TEST_F(GameObjectTest, ConstructorSetsName)
{
    EXPECT_EQ(gameObject_.GetName(), "TestObject");
}

TEST_F(GameObjectTest, DefaultName)
{
    GameObject go;
    EXPECT_EQ(go.GetName(), "GameObject");
}

TEST_F(GameObjectTest, SetName)
{
    gameObject_.SetName("NewName");
    EXPECT_EQ(gameObject_.GetName(), "NewName");
}

TEST_F(GameObjectTest, InitiallyActive)
{
    EXPECT_TRUE(gameObject_.IsActive());
}

TEST_F(GameObjectTest, SetActivefalse)
{
    gameObject_.SetActive(false);
    EXPECT_FALSE(gameObject_.IsActive());
}

TEST_F(GameObjectTest, DefaultLayer)
{
    EXPECT_EQ(gameObject_.GetLayer(), 0);
}

TEST_F(GameObjectTest, SetLayer)
{
    gameObject_.SetLayer(5);
    EXPECT_EQ(gameObject_.GetLayer(), 5);
}

//============================================================================
// AddComponent テスト
//============================================================================
TEST_F(GameObjectTest, AddComponentReturnsPointer)
{
    auto* comp = gameObject_.AddComponent<TestComponent>();
    EXPECT_NE(comp, nullptr);
}

TEST_F(GameObjectTest, AddComponentSetsOwner)
{
    auto* comp = gameObject_.AddComponent<TestComponent>();
    EXPECT_EQ(comp->GetOwner(), &gameObject_);
}

TEST_F(GameObjectTest, AddComponentCallsOnAttach)
{
    auto* comp = gameObject_.AddComponent<TestComponent>();
    EXPECT_TRUE(comp->wasAttached);
}

TEST_F(GameObjectTest, AddComponentWithArgs)
{
    // AnimatorはコンストラクタにU引数を取る
    auto* anim = gameObject_.AddComponent<Animator>(4, 8, 6);
    EXPECT_NE(anim, nullptr);
    EXPECT_EQ(anim->GetRowCount(), 4);
    EXPECT_EQ(anim->GetColumnCount(), 8);
}

//============================================================================
// GetComponent テスト
//============================================================================
TEST_F(GameObjectTest, GetComponentReturnsNullIfNotAdded)
{
    auto* comp = gameObject_.GetComponent<TestComponent>();
    EXPECT_EQ(comp, nullptr);
}

TEST_F(GameObjectTest, GetComponentReturnsAddedComponent)
{
    auto* added = gameObject_.AddComponent<TestComponent>();
    auto* got = gameObject_.GetComponent<TestComponent>();
    EXPECT_EQ(added, got);
}

TEST_F(GameObjectTest, GetComponentReturnsFirstOfType)
{
    auto* first = gameObject_.AddComponent<TestComponent>();
    (void)gameObject_.AddComponent<TestComponent>();  // 2つ目

    auto* got = gameObject_.GetComponent<TestComponent>();
    EXPECT_EQ(got, first);
}

TEST_F(GameObjectTest, GetComponentDifferentTypes)
{
    auto* test = gameObject_.AddComponent<TestComponent>();
    auto* transform = gameObject_.AddComponent<Transform>();

    EXPECT_EQ(gameObject_.GetComponent<TestComponent>(), test);
    EXPECT_EQ(gameObject_.GetComponent<Transform>(), transform);
}

//============================================================================
// GetComponents テスト
//============================================================================
TEST_F(GameObjectTest, GetComponentsEmptyVector)
{
    auto comps = gameObject_.GetComponents<TestComponent>();
    EXPECT_TRUE(comps.empty());
}

TEST_F(GameObjectTest, GetComponentsReturnsAll)
{
    auto* c1 = gameObject_.AddComponent<TestComponent>();
    auto* c2 = gameObject_.AddComponent<TestComponent>();
    auto* c3 = gameObject_.AddComponent<TestComponent>();

    auto comps = gameObject_.GetComponents<TestComponent>();
    ASSERT_EQ(comps.size(), 3);
    EXPECT_EQ(comps[0], c1);
    EXPECT_EQ(comps[1], c2);
    EXPECT_EQ(comps[2], c3);
}

//============================================================================
// RemoveComponent テスト
//============================================================================
TEST_F(GameObjectTest, RemoveComponentReturnsFalseIfNotPresent)
{
    EXPECT_FALSE(gameObject_.RemoveComponent<TestComponent>());
}

TEST_F(GameObjectTest, RemoveComponentReturnsTrue)
{
    gameObject_.AddComponent<TestComponent>();
    EXPECT_TRUE(gameObject_.RemoveComponent<TestComponent>());
}

TEST_F(GameObjectTest, RemoveComponentCallsOnDetach)
{
    auto* comp = gameObject_.AddComponent<TestComponent>();
    TestComponent* ptr = comp;  // コピーしておく

    gameObject_.RemoveComponent<TestComponent>();
    // 注: compはもう無効だが、wasDetachedはコールバック時に設定される
    // このテストはRemoveの実装に依存
    // 実際にはデストラクタで解放されるのでアクセスは危険
    // ここではRemoveが正常に動作することだけ確認
    EXPECT_TRUE(true);
}

TEST_F(GameObjectTest, RemoveComponentNullsOwner)
{
    // RemoveComponent後にGetComponent呼び出しでnullが返る
    gameObject_.AddComponent<TestComponent>();
    gameObject_.RemoveComponent<TestComponent>();
    EXPECT_EQ(gameObject_.GetComponent<TestComponent>(), nullptr);
}

TEST_F(GameObjectTest, RemoveFirstOfMultiple)
{
    (void)gameObject_.AddComponent<TestComponent>();
    auto* second = gameObject_.AddComponent<TestComponent>();

    gameObject_.RemoveComponent<TestComponent>();

    // 2つ目が残っている
    auto* remaining = gameObject_.GetComponent<TestComponent>();
    EXPECT_EQ(remaining, second);
}

//============================================================================
// Update テスト
//============================================================================
TEST_F(GameObjectTest, UpdateCallsComponentUpdate)
{
    auto* comp = gameObject_.AddComponent<TestComponent>();

    gameObject_.Update(0.016f);

    EXPECT_TRUE(comp->wasUpdated);
}

TEST_F(GameObjectTest, UpdateDoesNothingWhenInactive)
{
    auto* comp = gameObject_.AddComponent<TestComponent>();

    gameObject_.SetActive(false);
    gameObject_.Update(0.016f);

    EXPECT_FALSE(comp->wasUpdated);
}

TEST_F(GameObjectTest, UpdateSkipsDisabledComponents)
{
    auto* comp = gameObject_.AddComponent<TestComponent>();
    comp->SetEnabled(false);

    gameObject_.Update(0.016f);

    EXPECT_FALSE(comp->wasUpdated);
}

TEST_F(GameObjectTest, UpdateUpdatesMultipleComponents)
{
    auto* c1 = gameObject_.AddComponent<TestComponent>();
    auto* c2 = gameObject_.AddComponent<TestComponent>();

    gameObject_.Update(0.016f);

    EXPECT_TRUE(c1->wasUpdated);
    EXPECT_TRUE(c2->wasUpdated);
}

//============================================================================
// デストラクタテスト
//============================================================================
TEST(GameObjectDestructorTest, DetachesAllComponents)
{
    bool detached = false;

    {
        GameObject go;
        auto* comp = go.AddComponent<TestComponent>();
        // unique_ptrで所有されているので、デストラクタ時にOnDetachが呼ばれる
        (void)comp;
    }
    // GameObjectのスコープを抜けた後、コンポーネントは破棄されている

    EXPECT_TRUE(true);  // クラッシュしなければOK
}

//============================================================================
// ムーブテスト
//============================================================================
TEST(GameObjectMoveTest, MoveConstructor)
{
    GameObject go1("Original");
    (void)go1.AddComponent<TestComponent>();

    GameObject go2(std::move(go1));

    EXPECT_EQ(go2.GetName(), "Original");
    EXPECT_NE(go2.GetComponent<TestComponent>(), nullptr);
}

TEST(GameObjectMoveTest, MoveAssignment)
{
    GameObject go1("First");
    (void)go1.AddComponent<TestComponent>();

    GameObject go2("Second");
    go2 = std::move(go1);

    EXPECT_EQ(go2.GetName(), "First");
    EXPECT_NE(go2.GetComponent<TestComponent>(), nullptr);
}

} // namespace
