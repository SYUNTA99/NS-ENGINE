//----------------------------------------------------------------------------
//! @file   event_bus_test.cpp
//! @brief  EventBusのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/event/event_bus.h"
#include <string>
#include <vector>

namespace
{

//============================================================================
// テスト用イベント
//============================================================================
struct TestEvent {
    int value = 0;
};

struct StringEvent {
    std::string message;
};

struct AnotherEvent {
    float data = 0.0f;
};

//============================================================================
// EventHandler テスト
//============================================================================
class EventHandlerTest : public ::testing::Test
{
protected:
    EventHandler<TestEvent> handler_;
};

TEST_F(EventHandlerTest, InitiallyEmpty)
{
    EXPECT_TRUE(handler_.IsEmpty());
}

TEST_F(EventHandlerTest, AddMakesNonEmpty)
{
    handler_.Add(1, [](const TestEvent&) {});
    EXPECT_FALSE(handler_.IsEmpty());
}

TEST_F(EventHandlerTest, InvokeCallsCallback)
{
    int received = 0;
    handler_.Add(1, [&received](const TestEvent& e) {
        received = e.value;
    });

    TestEvent event{ 42 };
    handler_.Invoke(event);

    EXPECT_EQ(received, 42);
}

TEST_F(EventHandlerTest, InvokeCallsMultipleCallbacks)
{
    int count = 0;
    handler_.Add(1, [&count](const TestEvent&) { count++; });
    handler_.Add(2, [&count](const TestEvent&) { count++; });
    handler_.Add(3, [&count](const TestEvent&) { count++; });

    TestEvent event{};
    handler_.Invoke(event);

    EXPECT_EQ(count, 3);
}

TEST_F(EventHandlerTest, RemoveById)
{
    int count = 0;
    handler_.Add(1, [&count](const TestEvent&) { count++; });
    handler_.Add(2, [&count](const TestEvent&) { count++; });

    handler_.Remove(1);

    TestEvent event{};
    handler_.Invoke(event);

    EXPECT_EQ(count, 1);
}

TEST_F(EventHandlerTest, RemoveAllMakesEmpty)
{
    handler_.Add(1, [](const TestEvent&) {});
    handler_.Remove(1);
    EXPECT_TRUE(handler_.IsEmpty());
}

TEST_F(EventHandlerTest, PriorityOrder)
{
    std::vector<int> order;

    handler_.Add(1, [&order](const TestEvent&) { order.push_back(1); }, EventPriority::Low);
    handler_.Add(2, [&order](const TestEvent&) { order.push_back(2); }, EventPriority::High);
    handler_.Add(3, [&order](const TestEvent&) { order.push_back(3); }, EventPriority::Normal);

    TestEvent event{};
    handler_.Invoke(event);

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 2);  // High first
    EXPECT_EQ(order[1], 3);  // Normal second
    EXPECT_EQ(order[2], 1);  // Low last
}

//============================================================================
// EventBus シングルトンテスト
//============================================================================
class EventBusTest : public ::testing::Test
{
protected:
    void SetUp() override {
        EventBus::Create();
        EventBus::Get().Clear();
    }

    void TearDown() override {
        EventBus::Get().Clear();
    }
};

TEST_F(EventBusTest, SubscribeReturnsId)
{
    uint32_t id = EventBus::Get().Subscribe<TestEvent>([](const TestEvent&) {});
    EXPECT_GT(id, 0u);
}

TEST_F(EventBusTest, SubscribeReturnsDifferentIds)
{
    uint32_t id1 = EventBus::Get().Subscribe<TestEvent>([](const TestEvent&) {});
    uint32_t id2 = EventBus::Get().Subscribe<TestEvent>([](const TestEvent&) {});
    EXPECT_NE(id1, id2);
}

TEST_F(EventBusTest, PublishCallsSubscriber)
{
    int received = 0;
    EventBus::Get().Subscribe<TestEvent>([&received](const TestEvent& e) {
        received = e.value;
    });

    EventBus::Get().Publish(TestEvent{ 100 });

    EXPECT_EQ(received, 100);
}

TEST_F(EventBusTest, PublishCallsOnlyMatchingType)
{
    int testCount = 0;
    int anotherCount = 0;

    EventBus::Get().Subscribe<TestEvent>([&testCount](const TestEvent&) {
        testCount++;
    });
    EventBus::Get().Subscribe<AnotherEvent>([&anotherCount](const AnotherEvent&) {
        anotherCount++;
    });

    EventBus::Get().Publish(TestEvent{});

    EXPECT_EQ(testCount, 1);
    EXPECT_EQ(anotherCount, 0);
}

TEST_F(EventBusTest, UnsubscribeStopsCallbacks)
{
    int count = 0;
    uint32_t id = EventBus::Get().Subscribe<TestEvent>([&count](const TestEvent&) {
        count++;
    });

    EventBus::Get().Publish(TestEvent{});
    EXPECT_EQ(count, 1);

    EventBus::Get().Unsubscribe<TestEvent>(id);

    EventBus::Get().Publish(TestEvent{});
    EXPECT_EQ(count, 1);  // 増えない
}

TEST_F(EventBusTest, ClearRemovesAll)
{
    int count = 0;
    EventBus::Get().Subscribe<TestEvent>([&count](const TestEvent&) { count++; });
    EventBus::Get().Subscribe<AnotherEvent>([&count](const AnotherEvent&) { count++; });

    EventBus::Get().Clear();

    EventBus::Get().Publish(TestEvent{});
    EventBus::Get().Publish(AnotherEvent{});

    EXPECT_EQ(count, 0);
}

TEST_F(EventBusTest, PublishWithConstructorArgs)
{
    std::string received;
    EventBus::Get().Subscribe<StringEvent>([&received](const StringEvent& e) {
        received = e.message;
    });

    // テンプレート版Publish
    EventBus::Get().Publish<StringEvent>(StringEvent{"Hello"});

    EXPECT_EQ(received, "Hello");
}

TEST_F(EventBusTest, PriorityRespected)
{
    std::vector<int> order;

    EventBus::Get().Subscribe<TestEvent>(
        [&order](const TestEvent&) { order.push_back(1); },
        EventPriority::Low
    );
    EventBus::Get().Subscribe<TestEvent>(
        [&order](const TestEvent&) { order.push_back(2); },
        EventPriority::High
    );
    EventBus::Get().Subscribe<TestEvent>(
        [&order](const TestEvent&) { order.push_back(3); },
        EventPriority::Normal
    );

    EventBus::Get().Publish(TestEvent{});

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 2);  // High
    EXPECT_EQ(order[1], 3);  // Normal
    EXPECT_EQ(order[2], 1);  // Low
}

//============================================================================
// EventPriority テスト
//============================================================================
TEST(EventPriorityTest, HighIsLowestValue)
{
    EXPECT_LT(static_cast<uint8_t>(EventPriority::High),
              static_cast<uint8_t>(EventPriority::Normal));
}

TEST(EventPriorityTest, NormalIsBetween)
{
    EXPECT_GT(static_cast<uint8_t>(EventPriority::Normal),
              static_cast<uint8_t>(EventPriority::High));
    EXPECT_LT(static_cast<uint8_t>(EventPriority::Normal),
              static_cast<uint8_t>(EventPriority::Low));
}

} // namespace
