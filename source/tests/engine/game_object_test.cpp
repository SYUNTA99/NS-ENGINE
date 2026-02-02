//----------------------------------------------------------------------------
//! @file   game_object_test.cpp
//! @brief  GameObject（ECS Actorラッパー）のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_data.h"
#include "engine/ecs/components/rendering/sprite_data.h"
#include "engine/game_object/game_object_impl.h"

namespace
{

//============================================================================
// GameObject 基本テスト
//============================================================================
class GameObjectTest : public ::testing::Test
{
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        go_ = world_->CreateGameObject("TestObject");
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
    GameObject* go_ = nullptr;
};

TEST_F(GameObjectTest, CreateGameObjectReturnsValid)
{
    EXPECT_NE(go_, nullptr);
}

TEST_F(GameObjectTest, ConstructorSetsName)
{
    EXPECT_EQ(go_->GetName(), "TestObject");
}

TEST_F(GameObjectTest, DefaultName)
{
    GameObject* go = world_->CreateGameObject();
    EXPECT_EQ(go->GetName(), "GameObject");
}

TEST_F(GameObjectTest, SetName)
{
    go_->SetName("NewName");
    EXPECT_EQ(go_->GetName(), "NewName");
}

TEST_F(GameObjectTest, InitiallyActive)
{
    EXPECT_TRUE(go_->IsActive());
}

TEST_F(GameObjectTest, SetActiveFalse)
{
    go_->SetActive(false);
    EXPECT_FALSE(go_->IsActive());
}

TEST_F(GameObjectTest, HasValidActor)
{
    ECS::Actor actor = go_->GetActor();
    EXPECT_TRUE(actor.IsValid());
}

TEST_F(GameObjectTest, HasWorldReference)
{
    EXPECT_EQ(go_->GetWorld(), world_.get());
}

//============================================================================
// Add テスト
//============================================================================
TEST_F(GameObjectTest, AddComponentMakesHasReturnTrue)
{
    EXPECT_FALSE(go_->Has<ECS::TransformData>());
    go_->Add<ECS::TransformData>();
    EXPECT_TRUE(go_->Has<ECS::TransformData>());
}

TEST_F(GameObjectTest, AddComponentWithArgs)
{
    Vector3 pos(1.0f, 2.0f, 3.0f);
    Quaternion rot = Quaternion::Identity;
    Vector3 scale(2.0f, 2.0f, 2.0f);

    go_->Add<ECS::TransformData>(pos, rot, scale);

    EXPECT_TRUE(go_->Has<ECS::TransformData>());
    auto& t = go_->Get<ECS::TransformData>();
    EXPECT_FLOAT_EQ(t.position.x, 1.0f);
    EXPECT_FLOAT_EQ(t.position.y, 2.0f);
    EXPECT_FLOAT_EQ(t.position.z, 3.0f);
    EXPECT_FLOAT_EQ(t.scale.x, 2.0f);
}

TEST_F(GameObjectTest, AddMultipleComponentTypes)
{
    go_->Add<ECS::TransformData>();
    go_->Add<ECS::SpriteData>();

    EXPECT_TRUE(go_->Has<ECS::TransformData>());
    EXPECT_TRUE(go_->Has<ECS::SpriteData>());
}

//============================================================================
// Get テスト
//============================================================================
TEST_F(GameObjectTest, GetReturnsAddedComponent)
{
    go_->Add<ECS::TransformData>();
    auto& t = go_->Get<ECS::TransformData>();
    t.position.x = 100.0f;

    // 再度取得しても同じデータ
    auto& t2 = go_->Get<ECS::TransformData>();
    EXPECT_FLOAT_EQ(t2.position.x, 100.0f);
}

TEST_F(GameObjectTest, GetDifferentComponentTypes)
{
    go_->Add<ECS::TransformData>();
    go_->Add<ECS::SpriteData>();

    auto& t = go_->Get<ECS::TransformData>();
    auto& s = go_->Get<ECS::SpriteData>();

    // 両方アクセスできる
    t.position.x = 50.0f;
    s.visible = false;

    EXPECT_FLOAT_EQ(go_->Get<ECS::TransformData>().position.x, 50.0f);
    EXPECT_FALSE(go_->Get<ECS::SpriteData>().visible);
}

TEST_F(GameObjectTest, ConstGetReturnsConstReference)
{
    go_->Add<ECS::TransformData>();
    go_->Get<ECS::TransformData>().position.x = 42.0f;

    const GameObject* constGo = go_;
    const auto& t = constGo->Get<ECS::TransformData>();
    EXPECT_FLOAT_EQ(t.position.x, 42.0f);
}

//============================================================================
// Has テスト
//============================================================================
TEST_F(GameObjectTest, HasReturnsFalseIfNotAdded)
{
    EXPECT_FALSE(go_->Has<ECS::TransformData>());
}

TEST_F(GameObjectTest, HasReturnsTrueAfterAdd)
{
    go_->Add<ECS::TransformData>();
    EXPECT_TRUE(go_->Has<ECS::TransformData>());
}

TEST_F(GameObjectTest, HasIsTypeSpecific)
{
    go_->Add<ECS::TransformData>();
    EXPECT_TRUE(go_->Has<ECS::TransformData>());
    EXPECT_FALSE(go_->Has<ECS::SpriteData>());
}

//============================================================================
// Remove テスト
//============================================================================
TEST_F(GameObjectTest, RemoveMakesHasReturnFalse)
{
    go_->Add<ECS::TransformData>();
    EXPECT_TRUE(go_->Has<ECS::TransformData>());

    go_->Remove<ECS::TransformData>();
    EXPECT_FALSE(go_->Has<ECS::TransformData>());
}

TEST_F(GameObjectTest, RemoveDoesNotAffectOtherComponents)
{
    go_->Add<ECS::TransformData>();
    go_->Add<ECS::SpriteData>();

    go_->Remove<ECS::TransformData>();

    EXPECT_FALSE(go_->Has<ECS::TransformData>());
    EXPECT_TRUE(go_->Has<ECS::SpriteData>());
}

//============================================================================
// 複数GameObjectテスト
//============================================================================
TEST_F(GameObjectTest, MultipleGameObjectsAreIndependent)
{
    GameObject* go1 = world_->CreateGameObject("Object1");
    GameObject* go2 = world_->CreateGameObject("Object2");

    // Add all components first, then modify
    // (Adding components may reallocate storage, invalidating references)
    go1->Add<ECS::TransformData>();
    go2->Add<ECS::TransformData>();

    // Now modify - references are stable
    go1->Get<ECS::TransformData>().position.x = 10.0f;
    go2->Get<ECS::TransformData>().position.x = 20.0f;

    EXPECT_FLOAT_EQ(go1->Get<ECS::TransformData>().position.x, 10.0f);
    EXPECT_FLOAT_EQ(go2->Get<ECS::TransformData>().position.x, 20.0f);
}

TEST_F(GameObjectTest, EachGameObjectHasUniqueActor)
{
    GameObject* go1 = world_->CreateGameObject("Object1");
    GameObject* go2 = world_->CreateGameObject("Object2");

    EXPECT_NE(go1->GetActor().Index(), go2->GetActor().Index());
}

//============================================================================
// World連携テスト
//============================================================================
TEST_F(GameObjectTest, WorldCanAccessGameObjectComponents)
{
    go_->Add<ECS::TransformData>();
    go_->Get<ECS::TransformData>().position.x = 123.0f;

    // Worldから直接アクセスしても同じデータ
    auto* t = world_->GetComponent<ECS::TransformData>(go_->GetActor());
    ASSERT_NE(t, nullptr);
    EXPECT_FLOAT_EQ(t->position.x, 123.0f);
}

TEST_F(GameObjectTest, WorldForEachIncludesGameObjectComponents)
{
    go_->Add<ECS::TransformData>();
    go_->Get<ECS::TransformData>().position.x = 999.0f;

    bool found = false;
    world_->ForEach<ECS::TransformData>([&](ECS::Actor, ECS::TransformData& t) {
        if (t.position.x == 999.0f) {
            found = true;
        }
    });

    EXPECT_TRUE(found);
}

} // namespace
