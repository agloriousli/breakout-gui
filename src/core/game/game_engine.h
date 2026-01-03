#pragma once

#include <vector>
#include <memory>

#include "physics_engine.h"
#include "level_manager.h"
#include "endgame_state.h"
#include "../utils/random.h"
#include "../utils/vector2d.h"

namespace breakout {

class GameEngine {
public:
    GameEngine();

    enum class PowerupType { ExpandPaddle, ExtraLife, SpeedBoost, PointMultiplier, MultiBall };

    struct Powerup {
        PowerupType type;
        Vector2D position;
        Vector2D velocity;
        double size {14.0};
    };

    void setPlayfield(const Rect& bounds) { bounds_ = bounds; }
    void setLevels(const std::vector<std::vector<std::string>>& layouts) { levelManager_.setLayouts(layouts); }

    void setBallSpeed(double speed) { ballSpeed_ = speed; baseBallSpeed_ = speed; ball_.setSpeedPreserveDirection(speed); }
    void setRandomSeed(int seed);
    void setStartingLevel(int level) { startingLevel_ = level; }

    void newGame();
    void resetLevel(int levelIndex);
    void restartCurrentLevel();

    void attachBallToPaddle();
    void launchBall();
    bool isBallAttached() const { return ballAttached_; }

    void update(double deltaTime);

    bool levelComplete() const { return levelComplete_; }
    bool hasNextLevel() const { return levelManager_.hasLevel(currentLevel_ + 1); }
    bool advanceToNextLevel();

    void movePaddleLeft(double deltaTime) { paddle_.moveLeft(deltaTime, bounds_.left()); }
    void movePaddleRight(double deltaTime) { paddle_.moveRight(deltaTime, bounds_.right()); }

    const Ball& ball() const { return ball_; }
    const Paddle& paddle() const { return paddle_; }
    const std::vector<std::unique_ptr<Brick>>& bricks() const { return bricks_; }
    const std::vector<struct Powerup>& powerups() const { return powerups_; }
    Rect playfieldBounds() const { return bounds_; }

    int score() const { return score_; }
    int lives() const { return lives_; }
    int currentLevel() const { return currentLevel_; }
    int comboStreak() const { return comboStreak_; }
    int scoreMultiplier() const { return scoreMultiplier_; }
    double expandTimeRemaining() const { return expandTimer_; }
    double speedBoostTimeRemaining() const { return speedBoostTimer_; }
    int pointMultiplier() const { return pointMultiplier_; }
    double pointMultiplierTimeRemaining() const { return pointMultiplierTimer_; }
    double bigBallTimeRemaining() const { return bigBallTimer_; }
    bool isBigBallActive() const { return bigBallTimer_ > 0.0; }

    EndgameSnapshot snapshot(const std::string& name, const std::string& configName) const;
    void loadFromSnapshot(const EndgameSnapshot& state);

    bool isLevelComplete() const;
    bool isGameOver() const { return lives_ <= 0; }

private:
    void positionPaddleAndBall();
    int breakableBrickCount() const;
    void resetCombo();
    void spawnPowerup(const Vector2D& position);
    void spawnPowerupOfType(const Vector2D& position, PowerupType type);
    void updatePowerups(double deltaTime);
    void applyPowerup(const struct Powerup& p);
    void clearEffects();

    PhysicsEngine physics_;
    LevelManager levelManager_;
    Random rng_;

    Ball ball_ {8.0};
    Paddle paddle_ {110.0, 20.0, 280.0};
    std::vector<std::unique_ptr<Brick>> bricks_;
    std::vector<struct Powerup> powerups_;

    Rect bounds_ {0.0, 0.0, 640.0, 480.0};
    int score_ {0};
    int lives_ {3};
    int currentLevel_ {1};
    int startingLevel_ {1};
    double ballSpeed_ {260.0};
    double baseBallSpeed_ {260.0};
    bool ballAttached_ {false};
    bool levelComplete_ {false};
    int comboStreak_ {0};
    int scoreMultiplier_ {1};
    double expandTimer_ {0.0};
    double speedBoostTimer_ {0.0};
    int pointMultiplier_ {1};
    double pointMultiplierTimer_ {0.0};
    double levelBasePaddleWidth_ {110.0};
    double bigBallTimer_ {0.0};
    double baseBallRadius_ {8.0};
};

} // namespace breakout

/*
cmake -S . -B build-gui
cmake --build build-gui
./build-gui/breakout_app
*/