//----------------------------------------------------------------------------
//! @file   singleton_registry_test.cpp
//! @brief  SingletonRegistryのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/core/singleton_registry.h"

namespace
{

//============================================================================
// SingletonRegistry テスト
//============================================================================
class SingletonRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override {
        SingletonRegistry::Reset();
    }

    void TearDown() override {
        SingletonRegistry::Reset();
    }
};

TEST_F(SingletonRegistryTest, InitiallyAllCleared)
{
    EXPECT_TRUE(SingletonRegistry::AllCleared());
}

TEST_F(SingletonRegistryTest, RegisterWithNoDependencies)
{
    SingletonRegistry::Register(SingletonId::JobSystem, SingletonId::None, "JobSystem");
    EXPECT_TRUE(SingletonRegistry::IsInitialized(SingletonId::JobSystem));
}

TEST_F(SingletonRegistryTest, RegisterWithSatisfiedDependencies)
{
    SingletonRegistry::Register(SingletonId::GraphicsDevice, SingletonId::None, "GraphicsDevice");
    SingletonRegistry::Register(SingletonId::GraphicsContext, SingletonId::GraphicsDevice, "GraphicsContext");

    EXPECT_TRUE(SingletonRegistry::IsInitialized(SingletonId::GraphicsDevice));
    EXPECT_TRUE(SingletonRegistry::IsInitialized(SingletonId::GraphicsContext));
}

TEST_F(SingletonRegistryTest, UnregisterRemovesFromRegistry)
{
    SingletonRegistry::Register(SingletonId::JobSystem, SingletonId::None, "JobSystem");
    EXPECT_TRUE(SingletonRegistry::IsInitialized(SingletonId::JobSystem));

    SingletonRegistry::Unregister(SingletonId::JobSystem);
    EXPECT_FALSE(SingletonRegistry::IsInitialized(SingletonId::JobSystem));
}

TEST_F(SingletonRegistryTest, IsInitializedReturnsFalseForUnregistered)
{
    EXPECT_FALSE(SingletonRegistry::IsInitialized(SingletonId::TextureManager));
}

TEST_F(SingletonRegistryTest, ResetClearsAll)
{
    SingletonRegistry::Register(SingletonId::JobSystem, SingletonId::None, "JobSystem");
    SingletonRegistry::Register(SingletonId::FileSystemManager, SingletonId::None, "FileSystemManager");

    EXPECT_FALSE(SingletonRegistry::AllCleared());

    SingletonRegistry::Reset();

    EXPECT_TRUE(SingletonRegistry::AllCleared());
    EXPECT_FALSE(SingletonRegistry::IsInitialized(SingletonId::JobSystem));
    EXPECT_FALSE(SingletonRegistry::IsInitialized(SingletonId::FileSystemManager));
}

TEST_F(SingletonRegistryTest, MultipleDependencies)
{
    SingletonRegistry::Register(SingletonId::GraphicsDevice, SingletonId::None, "GraphicsDevice");
    SingletonRegistry::Register(SingletonId::GraphicsContext, SingletonId::None, "GraphicsContext");

    // TextureManagerはDeviceとContextの両方に依存
    SingletonId deps = SingletonId::GraphicsDevice | SingletonId::GraphicsContext;
    SingletonRegistry::Register(SingletonId::TextureManager, deps, "TextureManager");

    EXPECT_TRUE(SingletonRegistry::IsInitialized(SingletonId::TextureManager));
}

//============================================================================
// SingletonId ビット演算テスト
//============================================================================
TEST(SingletonIdTest, BitwiseOrCombinesFlags)
{
    SingletonId combined = SingletonId::GraphicsDevice | SingletonId::GraphicsContext;
    EXPECT_EQ(static_cast<uint32_t>(combined),
              static_cast<uint32_t>(SingletonId::GraphicsDevice) |
              static_cast<uint32_t>(SingletonId::GraphicsContext));
}

TEST(SingletonIdTest, BitwiseAndMasksFlags)
{
    SingletonId combined = SingletonId::GraphicsDevice | SingletonId::GraphicsContext;
    SingletonId result = combined & SingletonId::GraphicsDevice;
    EXPECT_EQ(result, SingletonId::GraphicsDevice);
}

TEST(SingletonIdTest, BitwiseNotInvertsFlags)
{
    SingletonId result = ~SingletonId::None;
    EXPECT_NE(static_cast<uint32_t>(result), 0u);
}

} // namespace
