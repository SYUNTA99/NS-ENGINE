//----------------------------------------------------------------------------
//! @file   dynamic_buffer_test.cpp
//! @brief  DynamicBuffer関連のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/buffer/buffer_element.h"
#include "engine/ecs/buffer/dynamic_buffer.h"

namespace {

//============================================================================
// テスト用バッファ要素型
//============================================================================

struct Waypoint : ECS::IBufferElement {
    float x, y, z;

    Waypoint() : x(0), y(0), z(0) {}
    Waypoint(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    bool operator==(const Waypoint& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};
ECS_BUFFER_ELEMENT(Waypoint);

struct SmallElement : ECS::IBufferElement {
    uint8_t value;

    SmallElement() : value(0) {}
    SmallElement(uint8_t v) : value(v) {}
};
ECS_BUFFER_ELEMENT(SmallElement);

struct LargeElement : ECS::IBufferElement {
    float data[16];  // 64 bytes

    LargeElement() { std::memset(data, 0, sizeof(data)); }
    LargeElement(float v) {
        for (int i = 0; i < 16; ++i) data[i] = v;
    }
};
ECS_BUFFER_ELEMENT(LargeElement);

//============================================================================
// テスト用通常コンポーネント（Archetype移動テスト用）
//============================================================================

struct TestPositionData : ECS::IComponentData {
    float x, y, z;

    TestPositionData() : x(0), y(0), z(0) {}
    TestPositionData(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};
ECS_COMPONENT(TestPositionData);

//============================================================================
// 基本操作テスト
//============================================================================

class DynamicBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(DynamicBufferTest, AddBufferAndAccess) {
    auto actor = world_->CreateActor();

    // バッファを追加
    auto buffer = world_->AddBuffer<Waypoint>(actor);
    EXPECT_TRUE(buffer.IsCreated());
    EXPECT_EQ(buffer.Length(), 0);
    EXPECT_GT(buffer.Capacity(), 0);

    // 要素を追加
    buffer.Add({1.0f, 2.0f, 3.0f});
    EXPECT_EQ(buffer.Length(), 1);
    EXPECT_EQ(buffer[0].x, 1.0f);
    EXPECT_EQ(buffer[0].y, 2.0f);
    EXPECT_EQ(buffer[0].z, 3.0f);

    // 複数要素を追加
    buffer.Add({4.0f, 5.0f, 6.0f});
    buffer.Add({7.0f, 8.0f, 9.0f});
    EXPECT_EQ(buffer.Length(), 3);
}

TEST_F(DynamicBufferTest, HasBuffer) {
    auto actor = world_->CreateActor();

    // バッファ追加前
    EXPECT_FALSE(world_->HasBuffer<Waypoint>(actor));

    // バッファ追加後
    world_->AddBuffer<Waypoint>(actor);
    EXPECT_TRUE(world_->HasBuffer<Waypoint>(actor));

    // 別のバッファ型はfalse
    EXPECT_FALSE(world_->HasBuffer<SmallElement>(actor));
}

TEST_F(DynamicBufferTest, GetBuffer) {
    auto actor = world_->CreateActor();
    world_->AddBuffer<Waypoint>(actor);

    // バッファを取得
    auto buffer = world_->GetBuffer<Waypoint>(actor);
    EXPECT_TRUE(buffer.IsCreated());

    buffer.Add({1.0f, 2.0f, 3.0f});

    // 再取得しても同じデータ
    auto buffer2 = world_->GetBuffer<Waypoint>(actor);
    EXPECT_EQ(buffer2.Length(), 1);
    EXPECT_EQ(buffer2[0].x, 1.0f);
}

TEST_F(DynamicBufferTest, GetBufferWithoutAddReturnsInvalid) {
    auto actor = world_->CreateActor();

    // バッファを追加していないActorからGetBufferすると無効なバッファを返す
    auto buffer = world_->GetBuffer<Waypoint>(actor);
    EXPECT_FALSE(buffer.IsCreated());
}

TEST_F(DynamicBufferTest, RemoveAt) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Add({1.0f, 0.0f, 0.0f});
    buffer.Add({2.0f, 0.0f, 0.0f});
    buffer.Add({3.0f, 0.0f, 0.0f});
    EXPECT_EQ(buffer.Length(), 3);

    // 中央の要素を削除
    buffer.RemoveAt(1);
    EXPECT_EQ(buffer.Length(), 2);
    EXPECT_EQ(buffer[0].x, 1.0f);
    EXPECT_EQ(buffer[1].x, 3.0f);  // 後続が詰められる
}

TEST_F(DynamicBufferTest, RemoveAtSwapBack) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Add({1.0f, 0.0f, 0.0f});
    buffer.Add({2.0f, 0.0f, 0.0f});
    buffer.Add({3.0f, 0.0f, 0.0f});
    EXPECT_EQ(buffer.Length(), 3);

    // 中央の要素を削除（swap-back）
    buffer.RemoveAtSwapBack(1);
    EXPECT_EQ(buffer.Length(), 2);
    EXPECT_EQ(buffer[0].x, 1.0f);
    EXPECT_EQ(buffer[1].x, 3.0f);  // 末尾と入れ替わる
}

TEST_F(DynamicBufferTest, Clear) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Add({1.0f, 2.0f, 3.0f});
    buffer.Add({4.0f, 5.0f, 6.0f});
    EXPECT_EQ(buffer.Length(), 2);

    buffer.Clear();
    EXPECT_EQ(buffer.Length(), 0);
    EXPECT_TRUE(buffer.IsEmpty());
}

TEST_F(DynamicBufferTest, FrontAndBack) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Add({1.0f, 0.0f, 0.0f});
    buffer.Add({2.0f, 0.0f, 0.0f});
    buffer.Add({3.0f, 0.0f, 0.0f});

    EXPECT_EQ(buffer.Front().x, 1.0f);
    EXPECT_EQ(buffer.Back().x, 3.0f);
}

//============================================================================
// 容量管理テスト
//============================================================================

TEST_F(DynamicBufferTest, GrowsWhenFull) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    int32_t initialCapacity = buffer.Capacity();

    // インライン容量を超えて追加
    for (int i = 0; i < initialCapacity + 10; ++i) {
        buffer.Add({static_cast<float>(i), 0.0f, 0.0f});
    }

    EXPECT_EQ(buffer.Length(), initialCapacity + 10);
    EXPECT_GE(buffer.Capacity(), initialCapacity + 10);

    // データが正しいか確認
    for (int i = 0; i < initialCapacity + 10; ++i) {
        EXPECT_EQ(buffer[i].x, static_cast<float>(i));
    }
}

TEST_F(DynamicBufferTest, EnsureCapacity) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    int32_t initialCapacity = buffer.Capacity();

    // 大きな容量を事前確保
    buffer.EnsureCapacity(1000);
    EXPECT_GE(buffer.Capacity(), 1000);

    // 要素はまだ追加されていない
    EXPECT_EQ(buffer.Length(), 0);
}

TEST_F(DynamicBufferTest, Resize) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Resize(5);
    EXPECT_EQ(buffer.Length(), 5);

    // デフォルト初期化されている
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(buffer[i].x, 0.0f);
        EXPECT_EQ(buffer[i].y, 0.0f);
        EXPECT_EQ(buffer[i].z, 0.0f);
    }

    // サイズを縮小
    buffer.Resize(2);
    EXPECT_EQ(buffer.Length(), 2);
}

TEST_F(DynamicBufferTest, ResizeUninitialized) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.ResizeUninitialized(10);
    EXPECT_EQ(buffer.Length(), 10);
    // 未初期化なので値は不定（テストしない）
}

//============================================================================
// イテレーションテスト
//============================================================================

TEST_F(DynamicBufferTest, RangeBasedFor) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Add({1.0f, 0.0f, 0.0f});
    buffer.Add({2.0f, 0.0f, 0.0f});
    buffer.Add({3.0f, 0.0f, 0.0f});

    float sum = 0.0f;
    for (const auto& wp : buffer) {
        sum += wp.x;
    }
    EXPECT_EQ(sum, 6.0f);
}

TEST_F(DynamicBufferTest, Iterators) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Add({1.0f, 0.0f, 0.0f});
    buffer.Add({2.0f, 0.0f, 0.0f});

    auto it = buffer.begin();
    EXPECT_EQ(it->x, 1.0f);
    ++it;
    EXPECT_EQ(it->x, 2.0f);
    ++it;
    EXPECT_EQ(it, buffer.end());
}

//============================================================================
// Archetype移動テスト
//============================================================================

TEST_F(DynamicBufferTest, MigrationWithInlineData) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    // インライン容量内のデータ
    buffer.Add({1.0f, 2.0f, 3.0f});
    buffer.Add({4.0f, 5.0f, 6.0f});

    // コンポーネント追加でArchetype移動
    world_->AddComponent<TestPositionData>(actor);

    // バッファデータが保持されている
    auto buffer2 = world_->GetBuffer<Waypoint>(actor);
    EXPECT_TRUE(buffer2.IsCreated());
    EXPECT_EQ(buffer2.Length(), 2);
    EXPECT_EQ(buffer2[0].x, 1.0f);
    EXPECT_EQ(buffer2[1].x, 4.0f);
}

TEST_F(DynamicBufferTest, MigrationWithExternalData) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    int32_t initialCapacity = buffer.Capacity();

    // インライン容量を超えて外部ストレージに移行
    for (int i = 0; i < initialCapacity + 20; ++i) {
        buffer.Add({static_cast<float>(i), 0.0f, 0.0f});
    }

    // コンポーネント追加でArchetype移動
    world_->AddComponent<TestPositionData>(actor);

    // バッファデータが保持されている（外部ストレージの所有権移譲）
    auto buffer2 = world_->GetBuffer<Waypoint>(actor);
    EXPECT_TRUE(buffer2.IsCreated());
    EXPECT_EQ(buffer2.Length(), initialCapacity + 20);

    for (int i = 0; i < initialCapacity + 20; ++i) {
        EXPECT_EQ(buffer2[i].x, static_cast<float>(i));
    }
}

//============================================================================
// メモリ管理テスト
//============================================================================

TEST_F(DynamicBufferTest, CleanupOnActorDestroy) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    // 外部ストレージに移行するまで追加
    for (int i = 0; i < 100; ++i) {
        buffer.Add({static_cast<float>(i), 0.0f, 0.0f});
    }

    // Actor破棄（外部ストレージも解放されるはず）
    world_->DestroyActor(actor);

    // メモリリークはAddressSanitizer等で検出
    // ここではActor破棄後にアクセスしないことを確認
    EXPECT_FALSE(world_->IsAlive(actor));
}

TEST_F(DynamicBufferTest, RemoveBuffer) {
    auto actor = world_->CreateActor();
    world_->AddBuffer<Waypoint>(actor);

    auto buffer = world_->GetBuffer<Waypoint>(actor);
    buffer.Add({1.0f, 2.0f, 3.0f});

    // バッファを削除
    world_->RemoveBuffer<Waypoint>(actor);

    EXPECT_FALSE(world_->HasBuffer<Waypoint>(actor));

    // 再取得すると無効なバッファ
    auto buffer2 = world_->GetBuffer<Waypoint>(actor);
    EXPECT_FALSE(buffer2.IsCreated());
}

//============================================================================
// 複数バッファテスト
//============================================================================

TEST_F(DynamicBufferTest, MultipleBufferTypes) {
    auto actor = world_->CreateActor();

    // 異なる型のバッファを追加
    // 注意: AddBufferはArchetype移動を引き起こすので、
    //       すべてのバッファを追加してから再取得する必要がある
    world_->AddBuffer<Waypoint>(actor);
    world_->AddBuffer<SmallElement>(actor);

    // すべてのバッファ追加後に再取得してデータを追加
    auto waypointBuffer = world_->GetBuffer<Waypoint>(actor);
    auto smallBuffer = world_->GetBuffer<SmallElement>(actor);

    waypointBuffer.Add({1.0f, 2.0f, 3.0f});
    smallBuffer.Add({42});
    smallBuffer.Add({100});

    EXPECT_EQ(waypointBuffer.Length(), 1);
    EXPECT_EQ(smallBuffer.Length(), 2);

    // 独立して動作
    EXPECT_EQ(world_->GetBuffer<Waypoint>(actor)[0].x, 1.0f);
    EXPECT_EQ(world_->GetBuffer<SmallElement>(actor)[0].value, 42);
    EXPECT_EQ(world_->GetBuffer<SmallElement>(actor)[1].value, 100);
}

//============================================================================
// インライン容量テスト
//============================================================================

TEST_F(DynamicBufferTest, InlineCapacityCalculation) {
    // Waypoint = 12 bytes
    // デフォルトインラインストレージ = 128 bytes
    // BufferHeader = 24 bytes
    // 容量 = (128 - 24) / 12 = 8
    constexpr int32_t expectedCapacity = (128 - 24) / 12;
    EXPECT_EQ(ECS::InternalBufferCapacity<Waypoint>::Value, expectedCapacity);

    // SmallElement = 1 byte
    // 容量 = (128 - 24) / 1 = 104
    constexpr int32_t expectedSmallCapacity = (128 - 24) / 1;
    EXPECT_EQ(ECS::InternalBufferCapacity<SmallElement>::Value, expectedSmallCapacity);

    // LargeElement = 64 bytes
    // 容量 = (128 - 24) / 64 = 1
    constexpr int32_t expectedLargeCapacity = (128 - 24) / 64;
    EXPECT_EQ(ECS::InternalBufferCapacity<LargeElement>::Value, expectedLargeCapacity);
}

//============================================================================
// AddDefaultテスト
//============================================================================

TEST_F(DynamicBufferTest, AddDefault) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    auto& wp = buffer.AddDefault();
    EXPECT_EQ(wp.x, 0.0f);
    EXPECT_EQ(wp.y, 0.0f);
    EXPECT_EQ(wp.z, 0.0f);

    // 変更可能
    wp.x = 10.0f;
    EXPECT_EQ(buffer[0].x, 10.0f);
}

//============================================================================
// bool変換テスト
//============================================================================

TEST_F(DynamicBufferTest, BoolConversion) {
    auto actor = world_->CreateActor();

    // 無効なバッファ
    auto invalidBuffer = world_->GetBuffer<Waypoint>(actor);
    EXPECT_FALSE(static_cast<bool>(invalidBuffer));
    EXPECT_FALSE(invalidBuffer);

    // 有効なバッファ
    auto validBuffer = world_->AddBuffer<Waypoint>(actor);
    EXPECT_TRUE(static_cast<bool>(validBuffer));
    EXPECT_TRUE(validBuffer);
}

//============================================================================
// Dataポインタテスト
//============================================================================

TEST_F(DynamicBufferTest, DataPointer) {
    auto actor = world_->CreateActor();
    auto buffer = world_->AddBuffer<Waypoint>(actor);

    buffer.Add({1.0f, 2.0f, 3.0f});
    buffer.Add({4.0f, 5.0f, 6.0f});

    Waypoint* data = buffer.Data();
    EXPECT_NE(data, nullptr);
    EXPECT_EQ(data[0].x, 1.0f);
    EXPECT_EQ(data[1].x, 4.0f);

    // const版
    const auto& constBuffer = buffer;
    const Waypoint* constData = constBuffer.Data();
    EXPECT_EQ(constData[0].x, 1.0f);
}

} // namespace
