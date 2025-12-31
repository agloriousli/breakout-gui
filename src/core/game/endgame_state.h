#pragma once

#include <string>
#include <vector>

#include "../utils/collision.h"
#include "../entities/ball.h"
#include "../entities/paddle.h"
#include "../entities/brick.h"

namespace breakout {

struct EndgameSnapshot {
    std::string name;
    std::string configName;
    int configBallSpeed {5};
    int configRandomSeed {-1};
    int configStartingLevel {1};
    int level {1};
    int score {0};
    int lives {3};
    int comboStreak {0};
    int scoreMultiplier {1};
    double expandTimer {0.0};
    double speedBoostTimer {0.0};
    double pointMultiplier {1.0};
    double pointMultiplierTimer {0.0};
    Rect bounds;
    BallState ball;
    PaddleState paddle;
    bool ballAttached {true};
    std::vector<BrickState> bricks;
    // Powerups on the field when saved
    struct SavedPowerup {
        int type {0};
        Vector2D position;
        Vector2D velocity;
        double size {14.0};
    };
    std::vector<SavedPowerup> powerups;
};

} // namespace breakout
