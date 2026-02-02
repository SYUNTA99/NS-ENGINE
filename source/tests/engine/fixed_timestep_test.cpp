//----------------------------------------------------------------------------
//! @file   fixed_timestep_test.cpp
//! @brief  固定タイムステップロジックのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <vector>

namespace
{

//============================================================================
// 固定タイムステップシミュレーター（Applicationのロジックを抽出）
//============================================================================
class FixedTimestepSimulator
{
public:
    float accumulator = 0.0f;
    float alpha = 0.0f;
    int fixedUpdateCount = 0;
    int renderCount = 0;
    std::vector<float> fixedUpdateDts;

    void SimulateFrame(float dt, float fixedDt, int maxIterations = 5) {
        accumulator += dt;

        int iterations = 0;
        while (accumulator >= fixedDt && iterations < maxIterations) {
            // FixedUpdate
            ++fixedUpdateCount;
            fixedUpdateDts.push_back(fixedDt);
            accumulator -= fixedDt;
            ++iterations;
        }

        // スパイク防止
        if (accumulator > fixedDt * 2.0f) {
            accumulator = fixedDt;
        }

        // 補間係数
        alpha = accumulator / fixedDt;

        // Render（常に1回）
        ++renderCount;
    }

    void Reset() {
        accumulator = 0.0f;
        alpha = 0.0f;
        fixedUpdateCount = 0;
        renderCount = 0;
        fixedUpdateDts.clear();
    }
};

//============================================================================
// テスト
//============================================================================
class FixedTimestepTest : public ::testing::Test
{
protected:
    FixedTimestepSimulator sim;
    static constexpr float kFixedDt = 1.0f / 60.0f;  // 約16.67ms
};

TEST_F(FixedTimestepTest, PerfectFrameRate)
{
    // 完璧な60FPS: 1フレームにつき1回のFixedUpdate
    for (int i = 0; i < 10; ++i) {
        sim.SimulateFrame(kFixedDt, kFixedDt);
    }

    EXPECT_EQ(sim.fixedUpdateCount, 10);
    EXPECT_EQ(sim.renderCount, 10);
    EXPECT_NEAR(sim.accumulator, 0.0f, 0.0001f);
}

TEST_F(FixedTimestepTest, SlowerFrameRate)
{
    // 30FPS: 1フレームにつき2回のFixedUpdate
    const float dt30fps = 1.0f / 30.0f;

    sim.SimulateFrame(dt30fps, kFixedDt);

    EXPECT_EQ(sim.fixedUpdateCount, 2);
    EXPECT_EQ(sim.renderCount, 1);
}

TEST_F(FixedTimestepTest, FasterFrameRate)
{
    // 120FPS: 2フレームに1回のFixedUpdate
    const float dt120fps = 1.0f / 120.0f;

    sim.SimulateFrame(dt120fps, kFixedDt);
    EXPECT_EQ(sim.fixedUpdateCount, 0);
    EXPECT_EQ(sim.renderCount, 1);

    sim.SimulateFrame(dt120fps, kFixedDt);
    EXPECT_EQ(sim.fixedUpdateCount, 1);
    EXPECT_EQ(sim.renderCount, 2);
}

TEST_F(FixedTimestepTest, AlphaInterpolation)
{
    // 半フレーム分
    sim.SimulateFrame(kFixedDt * 0.5f, kFixedDt);
    EXPECT_NEAR(sim.alpha, 0.5f, 0.01f);
    EXPECT_EQ(sim.fixedUpdateCount, 0);

    // さらに半フレーム分 → FixedUpdate実行
    sim.SimulateFrame(kFixedDt * 0.5f, kFixedDt);
    EXPECT_EQ(sim.fixedUpdateCount, 1);
    EXPECT_NEAR(sim.alpha, 0.0f, 0.01f);
}

TEST_F(FixedTimestepTest, SpikePrevention)
{
    // 大きなスパイク（ブレークポイントなど）
    const float hugeDt = 0.5f;  // 500ms

    sim.SimulateFrame(hugeDt, kFixedDt);

    // 最大5回に制限される
    EXPECT_EQ(sim.fixedUpdateCount, 5);
    // accumulatorは制限される
    EXPECT_LE(sim.accumulator, kFixedDt * 2.0f);
}

TEST_F(FixedTimestepTest, ConsistentDeltaTime)
{
    // 不規則なフレームレートでもFixedUpdateのdtは常に一定
    const float dts[] = { 0.008f, 0.02f, 0.015f, 0.033f, 0.016f };

    for (float dt : dts) {
        sim.SimulateFrame(dt, kFixedDt);
    }

    // 全てのFixedUpdateは同じdt
    for (float dt : sim.fixedUpdateDts) {
        EXPECT_FLOAT_EQ(dt, kFixedDt);
    }
}

TEST_F(FixedTimestepTest, VariableFrameRateStability)
{
    // 変動するフレームレートでも物理が安定
    sim.SimulateFrame(0.010f, kFixedDt);  // 100 FPS
    sim.SimulateFrame(0.033f, kFixedDt);  // 30 FPS
    sim.SimulateFrame(0.016f, kFixedDt);  // 60 FPS
    sim.SimulateFrame(0.050f, kFixedDt);  // 20 FPS

    // 合計約109msなので、約6-7回のFixedUpdate
    // 10ms + 33ms + 16ms + 50ms = 109ms
    // 109ms / 16.67ms ≈ 6.5回
    EXPECT_GE(sim.fixedUpdateCount, 6);
    EXPECT_LE(sim.fixedUpdateCount, 7);
}

TEST_F(FixedTimestepTest, AccumulatorPersistence)
{
    // アキュムレータが次のフレームに持ち越される
    sim.SimulateFrame(kFixedDt * 1.5f, kFixedDt);

    EXPECT_EQ(sim.fixedUpdateCount, 1);
    EXPECT_NEAR(sim.accumulator, kFixedDt * 0.5f, 0.0001f);

    sim.SimulateFrame(kFixedDt * 0.6f, kFixedDt);
    // 0.5 + 0.6 = 1.1 → 1回のFixedUpdate
    EXPECT_EQ(sim.fixedUpdateCount, 2);
}

TEST_F(FixedTimestepTest, ZeroDeltaTime)
{
    // dt=0でもクラッシュしない
    sim.SimulateFrame(0.0f, kFixedDt);

    EXPECT_EQ(sim.fixedUpdateCount, 0);
    EXPECT_EQ(sim.renderCount, 1);
}

TEST_F(FixedTimestepTest, VerySmallDeltaTime)
{
    // 非常に小さいdt
    for (int i = 0; i < 1000; ++i) {
        sim.SimulateFrame(0.001f, kFixedDt);  // 1ms
    }

    // 1000ms / 16.67ms ≈ 60回
    EXPECT_GE(sim.fixedUpdateCount, 59);
    EXPECT_LE(sim.fixedUpdateCount, 61);
}

TEST_F(FixedTimestepTest, MaxIterationsLimit)
{
    // maxIterations を1に制限
    sim.SimulateFrame(kFixedDt * 10.0f, kFixedDt, 1);

    // 最大1回
    EXPECT_EQ(sim.fixedUpdateCount, 1);
}

} // namespace
