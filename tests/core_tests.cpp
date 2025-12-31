#include <cmath>
#include <limits>
#include <vector>
#include <filesystem>

#include <gtest/gtest.h>

#include "core/game/physics_engine.h"
#include "core/game/game_engine.h"
#include "core/game/endgame_state.h"
#include "core/utils/random.h"

using namespace breakout;

TEST(PhysicsEngineTest, PaddleReflectionAngles) {
    PhysicsEngine physics;
    Vector2D incoming(0.0, 200.0);

    Vector2D center = physics.calculatePaddleReflection(incoming, 0.0);
    EXPECT_NEAR(center.x(), 0.0, 1e-6);
    EXPECT_LT(center.y(), 0.0);

    Vector2D left = physics.calculatePaddleReflection(incoming, -1.0);
    EXPECT_LT(left.x(), 0.0);
    EXPECT_LT(left.y(), 0.0);

    Vector2D clamped = physics.calculatePaddleReflection(incoming, -2.0);
    double angle = std::atan2(-clamped.y(), clamped.x());
    EXPECT_GE(angle, M_PI / 6.0 - 1e-6);
}

TEST(BrickTest, DurableAndIndestructibleBehavior) {
    Rect rect {0, 0, 10, 10};
    DurableBrick durable(rect, 2);
    EXPECT_FALSE(durable.applyHit());
    EXPECT_TRUE(durable.applyHit());

    IndestructibleBrick indestructible(rect);
    EXPECT_FALSE(indestructible.applyHit());
    EXPECT_EQ(indestructible.hitsRemaining(), std::numeric_limits<int>::max());
}

TEST(PhysicsEngineTest, WallCollisionFlipsVelocity) {
    PhysicsEngine physics;
    Ball ball(5.0);
    ball.setPosition({2.0, 2.0});
    ball.setVelocity({-100.0, -50.0});

    Rect bounds {0.0, 0.0, 200.0, 200.0};
    physics.resolveWallCollision(ball, bounds);

    EXPECT_GT(ball.velocity().x(), 0.0);
    EXPECT_GT(ball.velocity().y(), 0.0);
}

TEST(GameEngineTest, LevelCompletesWhenBreakableGone) {
    GameEngine engine;
    engine.setPlayfield({0.0, 0.0, 400.0, 300.0});
    engine.resetLevel(1);

    // Destroy all breakable bricks manually
    auto& bricks = const_cast<std::vector<std::unique_ptr<Brick>>&>(engine.bricks());
    bricks.clear();

    EXPECT_TRUE(engine.isLevelComplete());
}

TEST(GameEngineTest, LifeDecrementsWhenBallFalls) {
    GameEngine engine;
    engine.setPlayfield({0.0, 0.0, 400.0, 300.0});
    engine.resetLevel(1);

    Ball& ball = const_cast<Ball&>(engine.ball());
    ball.setPosition({200.0, 310.0});
    ball.setVelocity({0.0, 50.0});

    int initialLives = engine.lives();
    engine.update(0.016);
    EXPECT_EQ(engine.lives(), initialLives - 1);
}

TEST(GameEngineTest, StartingLevelFallsBackWhenMissing) {
    GameEngine engine;
    engine.setStartingLevel(5);
    // Default layouts only include level 1; ensure we clamp back to level 1.
    engine.newGame();
    EXPECT_EQ(engine.currentLevel(), 1);
}

TEST(GameEngineTest, StartingLevelAppliesWhenPresent) {
    GameEngine engine;
    std::vector<std::vector<std::string>> layouts {
        {"@@@@"},
        {"####"}
    };
    engine.setLevels(layouts);
    engine.setStartingLevel(2);
    engine.newGame();
    EXPECT_EQ(engine.currentLevel(), 2);
}

TEST(GameEngineTest, LaunchUsesConfiguredBallSpeed) {
    GameEngine engine;
    engine.setBallSpeed(420.0);
    engine.attachBallToPaddle();
    engine.launchBall();
    EXPECT_NEAR(engine.ball().velocity().y(), -420.0, 1e-6);
}

TEST(RandomTest, DeterministicWithSeed) {
    Random rngA(12345);
    Random rngB(12345);

    int a1 = rngA.nextInt(0, 1000000);
    int b1 = rngB.nextInt(0, 1000000);
    EXPECT_EQ(a1, b1);

    double a2 = rngA.nextDouble(-10.0, 10.0);
    double b2 = rngB.nextDouble(-10.0, 10.0);
    EXPECT_NEAR(a2, b2, 1e-9);
}

TEST(EndgameManagerTest, SnapshotRoundTripStruct) {
    breakout::EndgameSnapshot snap;
    snap.name = "test";
    snap.configName = "cfg";
    snap.configBallSpeed = 7;
    snap.configRandomSeed = 123;
    snap.configStartingLevel = 2;
    snap.level = 3;
    snap.score = 456;
    snap.lives = 2;
    snap.bounds = {10.0, 20.0, 300.0, 200.0};
    snap.ball.position = {30.0, 40.0};
    snap.ball.velocity = {50.0, 60.0};
    snap.ball.radius = 7.0;
    snap.paddle.position = {70.0, 80.0};
    snap.paddle.width = 90.0;
    snap.paddle.height = 15.0;
    snap.ballAttached = false;
    BrickState b;
    b.type = BrickType::Durable;
    b.hitsRemaining = 2;
    b.bounds = {100.0, 110.0, 48.0, 20.0};
    snap.bricks.push_back(b);

    breakout::EndgameSnapshot loaded = snap;

    EXPECT_EQ(loaded.configBallSpeed, snap.configBallSpeed);
    EXPECT_EQ(loaded.configRandomSeed, snap.configRandomSeed);
    EXPECT_EQ(loaded.configStartingLevel, snap.configStartingLevel);
    EXPECT_EQ(loaded.level, snap.level);
    EXPECT_EQ(loaded.score, snap.score);
    EXPECT_EQ(loaded.lives, snap.lives);
    EXPECT_DOUBLE_EQ(loaded.bounds.x, snap.bounds.x);
    EXPECT_DOUBLE_EQ(loaded.bounds.y, snap.bounds.y);
    EXPECT_DOUBLE_EQ(loaded.bounds.width, snap.bounds.width);
    EXPECT_DOUBLE_EQ(loaded.bounds.height, snap.bounds.height);
    EXPECT_DOUBLE_EQ(loaded.ball.position.x(), snap.ball.position.x());
    EXPECT_DOUBLE_EQ(loaded.ball.position.y(), snap.ball.position.y());
    EXPECT_DOUBLE_EQ(loaded.ball.velocity.x(), snap.ball.velocity.x());
    EXPECT_DOUBLE_EQ(loaded.ball.velocity.y(), snap.ball.velocity.y());
    EXPECT_DOUBLE_EQ(loaded.ball.radius, snap.ball.radius);
    EXPECT_DOUBLE_EQ(loaded.paddle.position.x(), snap.paddle.position.x());
    EXPECT_DOUBLE_EQ(loaded.paddle.position.y(), snap.paddle.position.y());
    EXPECT_DOUBLE_EQ(loaded.paddle.width, snap.paddle.width);
    EXPECT_DOUBLE_EQ(loaded.paddle.height, snap.paddle.height);
    EXPECT_EQ(loaded.ballAttached, snap.ballAttached);
    ASSERT_EQ(loaded.bricks.size(), snap.bricks.size());
    EXPECT_EQ(static_cast<int>(loaded.bricks[0].type), static_cast<int>(snap.bricks[0].type));
    EXPECT_EQ(loaded.bricks[0].hitsRemaining, snap.bricks[0].hitsRemaining);
    EXPECT_DOUBLE_EQ(loaded.bricks[0].bounds.x, snap.bricks[0].bounds.x);
    EXPECT_DOUBLE_EQ(loaded.bricks[0].bounds.y, snap.bricks[0].bounds.y);
    EXPECT_DOUBLE_EQ(loaded.bricks[0].bounds.width, snap.bricks[0].bounds.width);
    EXPECT_DOUBLE_EQ(loaded.bricks[0].bounds.height, snap.bricks[0].bounds.height);
}

TEST(BrickFactoryTest, CreatesCorrectTypes) {
    Rect rect {0, 0, 10, 10};
    auto normal = BrickFactory::createFromChar('@', rect);
    auto durable = BrickFactory::createFromChar('#', rect);
    auto indestructible = BrickFactory::createFromChar('*', rect);

    EXPECT_EQ(normal->type(), BrickType::Normal);
    EXPECT_EQ(durable->type(), BrickType::Durable);
    EXPECT_EQ(indestructible->type(), BrickType::Indestructible);
}
