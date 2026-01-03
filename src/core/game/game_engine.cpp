#include "game_engine.h"

#include <algorithm>
#include <cmath>
#include <memory>

using namespace std;

namespace breakout {

namespace {
// ============================================================================
// Helper Functions
// ============================================================================

// Create a brick from saved state
unique_ptr<Brick> createBrickFromState(const BrickState& state) {
    unique_ptr<Brick> brick;
    switch (state.type) {
    case BrickType::Normal: 
        brick = make_unique<NormalBrick>(state.bounds);
        break;
    case BrickType::Durable: 
        brick = make_unique<DurableBrick>(state.bounds, state.hitsRemaining);
        break;
    case BrickType::Indestructible: 
        brick = make_unique<IndestructibleBrick>(state.bounds);
        break;
    default:
        return nullptr;
    }
    // Restore the destroyed state from the saved snapshot
    brick->restoreState(state);
    return brick;
}

// ============================================================================
// Game Constants
// ============================================================================
constexpr double kPowerupSpawnChance = 0.5;      // 100% chance to spawn powerup
constexpr double kPowerupFallSpeed = 120.0;      // Pixels per second
constexpr double kExpandWidthBonus = 70.0;       // Extra paddle width (pixels)
constexpr double kExpandDuration = 12.0;         // Expand effect duration (seconds)
constexpr double kSpeedBoostDuration = 10.0;     // Speed boost duration (seconds)
constexpr double kSpeedBoostMultiplier = 1.5;    // Speed boost multiplier
constexpr double kPointMultiplierDuration = 15.0; // Point multiplier duration (seconds)
constexpr double kMaxPaddleWidth = 320.0;        // Maximum paddle width (pixels) - allows expansion
constexpr double kBrickPoints = 100.0;           // Points per normal brick
constexpr int kMaxLives = 5;                     // Maximum lives
constexpr int kMaxPointMultiplier = 10;          // Maximum point multiplier

// Clamp value between min and max
double clamp(double v, double minVal, double maxVal) {
    return max(minVal, min(v, maxVal));
}
} // namespace

// ============================================================================
// GameEngine Constructor - Initialize game with default levels
// ============================================================================
GameEngine::GameEngine() {
    if (levelManager_.levelCount() == 0) {
        vector<vector<string>> defaultLayouts {
            // Level 1: Simple pattern
            {"@@@@@@@@@@@@", "@#@#@#@#@#@#", "@@@@@***@@@@"},
            // Level 2: More rows, strategic indestructible placement
            {"@@@***@@@***", "@#@#@#@#@#@#", "@@@@@@@@@@@@", "@#@#@#@#@#@#", "@@@***@@@***"},
            // Level 3: Complex pattern with walls
            {"*@@@@@@@@@@*", "@#########@", "@@@@@@@@@@@@", "@##*##*##*@", "*@@@@@@@@@@*"}
        };
        levelManager_.setLayouts(defaultLayouts);
    }
    newGame();
}

// ============================================================================
// Game Initialization and Level Management
// ============================================================================
void GameEngine::newGame() {
    // Reset game state
    score_ = 0;
    lives_ = 3;
    levelComplete_ = false;
    comboStreak_ = 0;
    scoreMultiplier_ = 1;
    clearEffects();
    powerups_.clear();
    
    // Set starting level
    int startLevel = max(1, startingLevel_);
    if (!levelManager_.hasLevel(startLevel)) {
        startLevel = 1;
    }
    currentLevel_ = startLevel;
    
    resetLevel(currentLevel_);
    attachBallToPaddle();
}

void GameEngine::resetLevel(int levelIndex) {
    // Reset level-specific state
    currentLevel_ = levelIndex;
    levelComplete_ = false;
    comboStreak_ = 0;
    scoreMultiplier_ = 1;
    clearEffects();
    powerups_.clear();
    
    // Calculate brick dimensions to fit within playfield bounds
    size_t maxCols = levelManager_.maxColumns(levelIndex);
    if (maxCols == 0) maxCols = 12; // fallback
    
    double availableWidth = bounds_.width - 16.0;  // Account for margins
    double brickWidth = availableWidth / static_cast<double>(maxCols);
    double brickHeight = 28.0;
    double offsetX = bounds_.x + 8.0;
    double offsetY = bounds_.y + 8.0;

    // Adjust paddle size based on difficulty - shrinks as levels increase
    const double basePaddleWidth = 200.0;
    const double shrinkPerLevel = 20.0;
    const double minPaddleWidth = 100.0;
    double newPaddleWidth = max(minPaddleWidth, basePaddleWidth - (levelIndex - 1) * shrinkPerLevel);
    levelBasePaddleWidth_ = newPaddleWidth;
    paddle_.setSize(newPaddleWidth, paddle_.height());
    
    // Build brick layout and position game objects
    bricks_ = levelManager_.buildLevel(levelIndex, brickWidth, brickHeight, offsetX, offsetY);
    positionPaddleAndBall();
    ballAttached_ = false;
}

void GameEngine::restartCurrentLevel() {
    resetLevel(currentLevel_);
    attachBallToPaddle();
}

void GameEngine::attachBallToPaddle() {
    ballAttached_ = true;
    ball_.setVelocity({0.0, 0.0});
    ball_.setPosition({ paddle_.position().x() + paddle_.width() * 0.5, paddle_.position().y() - ball_.radius() - 1.0 });
}

void GameEngine::launchBall() {
    if (!ballAttached_) {
        return;
    }
    ballAttached_ = false;
    ball_.setVelocity({0.0, -ballSpeed_});
}

void GameEngine::setRandomSeed(int seed) {
    if (seed < 0) {
        rng_ = Random();
    } else {
        rng_ = Random(seed);
    }
}

void GameEngine::positionPaddleAndBall() {
    double paddleY = bounds_.bottom() - paddle_.height() - 12.0;
    double paddleX = bounds_.x + bounds_.width * 0.5 - paddle_.width() * 0.5;
    paddle_.setPosition({ paddleX, paddleY });
    ball_.setPosition({ paddle_.position().x() + paddle_.width() * 0.5, paddle_.position().y() - ball_.radius() - 1.0 });
    ball_.setVelocity({0.0, -ballSpeed_});
}

int GameEngine::breakableBrickCount() const {
    int count = 0;
    for (const auto& brick : bricks_) {
        if (brick->isBreakable() && !brick->isDestroyed()) {
            ++count;
        }
    }
    return count;
}

bool GameEngine::isLevelComplete() const {
    return breakableBrickCount() == 0;
}

void GameEngine::resetCombo() {
    comboStreak_ = 0;
    scoreMultiplier_ = 1;
}

void GameEngine::update(double deltaTime) {
    if (isGameOver() || levelComplete_) {
        return;
    }

    updatePowerups(deltaTime);

    if (ballAttached_) {
        ball_.setPosition({ paddle_.position().x() + paddle_.width() * 0.5, paddle_.position().y() - ball_.radius() - 1.0 });
        return;
    }

    // CHECK IF BALL FELL OFF BOTTOM BEFORE PROCESSING PHYSICS
    // This prevents the physics engine from processing collisions after the ball is lost
    if (ball_.bounds().bottom() >= bounds_.bottom()) {
        --lives_;
        resetCombo();
        if (!isGameOver()) {
            positionPaddleAndBall();
            attachBallToPaddle();
        }
        return;
    }

    // Track which bricks are already destroyed before collision resolution
    vector<bool> wasDestroyed(bricks_.size());
    for (size_t i = 0; i < bricks_.size(); ++i) {
        wasDestroyed[i] = bricks_[i]->isDestroyed();
    }

    int destroyed = physics_.resolveBrickCollisions(ball_, bricks_, deltaTime, bigBallTimer_ > 0.0);

    if (destroyed > 0) {
        comboStreak_ += destroyed;
        scoreMultiplier_ = std::clamp(1 + comboStreak_ / 3, 1, 5);
        score_ += destroyed * static_cast<int>(kBrickPoints) * scoreMultiplier_ * pointMultiplier_;
        
        // Check each brick for newly destroyed status and spawn assigned powerups
        for (size_t i = 0; i < bricks_.size(); ++i) {
            if (!wasDestroyed[i] && bricks_[i]->isDestroyed()) {
                // This brick was just destroyed
                int assignedPowerup = bricks_[i]->assignedPowerup();
                if (assignedPowerup >= 0 && assignedPowerup <= 4) {
                    // Spawn the specific assigned powerup
                    spawnPowerupOfType(bricks_[i]->bounds().center(), static_cast<PowerupType>(assignedPowerup));
                } else if (rng_.nextDouble(0.0, 1.0) < kPowerupSpawnChance) {
                    // No assigned powerup, use random spawn chance
                    spawnPowerup(bricks_[i]->bounds().center());
                }
            }
        }
    }

    physics_.resolveWallCollision(ball_, bounds_);

    bool paddleHit = physics_.resolvePaddleCollision(ball_, paddle_);
    if (paddleHit) {
        resetCombo();
    }

    if (isLevelComplete()) {
        levelComplete_ = true;
        attachBallToPaddle();
    }
}

void GameEngine::spawnPowerup(const Vector2D& position) {
    Powerup p;
    // Randomly select from 5 powerup types
    double roll = rng_.nextDouble(0.0, 1.0);
    if (roll < 0.2) p.type = PowerupType::ExpandPaddle;
    else if (roll < 0.4) p.type = PowerupType::ExtraLife;
    else if (roll < 0.6) p.type = PowerupType::SpeedBoost;
    else if (roll < 0.8) p.type = PowerupType::PointMultiplier;
    else p.type = PowerupType::MultiBall;
    
    p.position = position;
    p.velocity = {0.0, kPowerupFallSpeed};
    powerups_.push_back(p);
}

void GameEngine::spawnPowerupOfType(const Vector2D& position, PowerupType type) {
    Powerup p;
    p.type = type;
    p.position = position;
    p.velocity = {0.0, kPowerupFallSpeed};
    powerups_.push_back(p);
}

void GameEngine::applyPowerup(const Powerup& p) {
    switch (p.type) {
    case PowerupType::ExpandPaddle: {
        double centerX = paddle_.position().x() + paddle_.width() * 0.5;
        double targetWidth = clamp(levelBasePaddleWidth_ + kExpandWidthBonus, levelBasePaddleWidth_, kMaxPaddleWidth);
        paddle_.setSize(targetWidth, paddle_.height());
        double newX = centerX - targetWidth * 0.5;
        // Clamp paddle position to stay within playfield bounds
        newX = clamp(newX, bounds_.left(), bounds_.right() - targetWidth);
        paddle_.setPosition({ newX, paddle_.position().y() });
        // Stack duration with maximum cap of 60 seconds
        expandTimer_ = std::min(expandTimer_ + kExpandDuration, 60.0);
        break;
    }
    case PowerupType::ExtraLife: {
        lives_ = std::min(lives_ + 1, kMaxLives);
        break;
    }
    case PowerupType::SpeedBoost: {
        // Stack duration with maximum cap of 60 seconds
        speedBoostTimer_ = std::min(speedBoostTimer_ + kSpeedBoostDuration, 60.0);
        ball_.setSpeedPreserveDirection(baseBallSpeed_ * kSpeedBoostMultiplier);
        break;
    }
    case PowerupType::PointMultiplier: {
        // Add +2 to multiplier, capped at max
        pointMultiplier_ = std::min(pointMultiplier_ + 2, kMaxPointMultiplier);
        // Stack duration with maximum cap of 60 seconds
        pointMultiplierTimer_ = std::min(pointMultiplierTimer_ + kPointMultiplierDuration, 60.0);
        break;
    }
    case PowerupType::MultiBall: {
        // Big ball implementation - increase ball size and destroy multiple blocks
        bigBallTimer_ = 15.0;  // 15 seconds duration
        ball_.setRadius(baseBallRadius_ * 2.0);  // Double the ball size
        break;
    }
    }
}

void GameEngine::updatePowerups(double deltaTime) {
    // Update effect timers
    if (expandTimer_ > 0.0) {
        expandTimer_ -= deltaTime;
        if (expandTimer_ <= 0.0) {
            double centerX = paddle_.position().x() + paddle_.width() * 0.5;
            paddle_.setSize(levelBasePaddleWidth_, paddle_.height());
            paddle_.setPosition({ centerX - levelBasePaddleWidth_ * 0.5, paddle_.position().y() });
            expandTimer_ = 0.0;
        }
    }
    if (speedBoostTimer_ > 0.0) {
        speedBoostTimer_ -= deltaTime;
        if (speedBoostTimer_ <= 0.0) {
            ball_.setSpeedPreserveDirection(baseBallSpeed_);
            speedBoostTimer_ = 0.0;
        }
    }
    if (pointMultiplierTimer_ > 0.0) {
        pointMultiplierTimer_ -= deltaTime;
        if (pointMultiplierTimer_ <= 0.0) {
            pointMultiplier_ = 1;
            pointMultiplierTimer_ = 0.0;
        }
    }

    if (bigBallTimer_ > 0.0) {
        bigBallTimer_ -= deltaTime;
        if (bigBallTimer_ <= 0.0) {
            bigBallTimer_ = 0.0;
            ball_.setRadius(baseBallRadius_);  // Reset to normal size
        }
    }

    if (powerups_.empty()) return;

    std::vector<Powerup> next;
    next.reserve(powerups_.size());
    Rect paddleRect = paddle_.bounds();
    for (auto& p : powerups_) {
        p.position += p.velocity * deltaTime;
        Rect pr { p.position.x() - p.size * 0.5, p.position.y() - p.size * 0.5, p.size, p.size };
        // Collect
        if (intersects(pr, paddleRect)) {
            applyPowerup(p);
            continue;
        }
        // Missed and fell out
        if (pr.top() > bounds_.bottom()) {
            continue;
        }
        next.push_back(p);
    }
    powerups_.swap(next);
}

void GameEngine::clearEffects() {
    expandTimer_ = 0.0;
    speedBoostTimer_ = 0.0;
    pointMultiplier_ = 1;
    pointMultiplierTimer_ = 0.0;
    bigBallTimer_ = 0.0;
    double centerX = paddle_.position().x() + paddle_.width() * 0.5;
    paddle_.setSize(levelBasePaddleWidth_, paddle_.height());
    paddle_.setPosition({ centerX - levelBasePaddleWidth_ * 0.5, paddle_.position().y() });
    ball_.setSpeedPreserveDirection(baseBallSpeed_);
    ball_.setRadius(baseBallRadius_);
}

bool GameEngine::advanceToNextLevel() {
    if (!levelManager_.hasLevel(currentLevel_ + 1)) {
        return false;
    }
    // Reset destroyed flags before building the new level
    for (auto& brick : bricks_) {
        if (brick->isDestroyed()) {
            BrickState state = brick->state();
            state.destroyed = false;
            brick->restoreState(state);
        }
    }
    resetLevel(currentLevel_ + 1);
    attachBallToPaddle();
    levelComplete_ = false;
    return true;
}

EndgameSnapshot GameEngine::snapshot(const std::string& name, const std::string& configName) const {
    EndgameSnapshot snap;
    snap.name = name;
    snap.configName = configName;
    snap.level = currentLevel_;
    snap.score = score_;
    snap.lives = lives_;
    snap.comboStreak = comboStreak_;
    snap.scoreMultiplier = scoreMultiplier_;
    snap.expandTimer = expandTimer_;
    snap.speedBoostTimer = speedBoostTimer_;
    snap.pointMultiplier = pointMultiplier_;
    snap.pointMultiplierTimer = pointMultiplierTimer_;
    snap.bounds = bounds_;
    snap.ball = ball_.state();
    snap.paddle = paddle_.state();
    snap.ballAttached = ballAttached_;
    snap.bricks.reserve(bricks_.size());
    for (const auto& brick : bricks_) {
        snap.bricks.push_back(brick->state());
    }
    snap.powerups.reserve(powerups_.size());
    for (const auto& p : powerups_) {
        EndgameSnapshot::SavedPowerup saved;
        saved.type = static_cast<int>(p.type);
        saved.position = p.position;
        saved.velocity = p.velocity;
        saved.size = p.size;
        snap.powerups.push_back(saved);
    }
    return snap;
}

void GameEngine::loadFromSnapshot(const EndgameSnapshot& state) {
    bounds_ = state.bounds;
    score_ = state.score;
    lives_ = state.lives;
    currentLevel_ = state.level;
    ball_.restore(state.ball);
    paddle_.restore(state.paddle);
    ballAttached_ = state.ballAttached;
    levelComplete_ = false;
    comboStreak_ = state.comboStreak;
    scoreMultiplier_ = state.scoreMultiplier;
    expandTimer_ = state.expandTimer;
    speedBoostTimer_ = state.speedBoostTimer;
    pointMultiplier_ = state.pointMultiplier;
    pointMultiplierTimer_ = state.pointMultiplierTimer;
    levelBasePaddleWidth_ = paddle_.width();
    powerups_.clear();
    for (const auto& saved : state.powerups) {
        Powerup p;
        // Map old save format: 0=ExpandPaddle, other types default to ExpandPaddle for compatibility
        p.type = PowerupType::ExpandPaddle;
        p.position = saved.position;
        p.velocity = saved.velocity;
        p.size = saved.size;
        powerups_.push_back(p);
    }

    bricks_.clear();
    for (const auto& bState : state.bricks) {
        auto b = createBrickFromState(bState);
        if (b) {
            bricks_.push_back(std::move(b));
        }
    }
}

} // namespace breakout
