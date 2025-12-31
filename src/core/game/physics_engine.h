#pragma once

#include <vector>

#include "../entities/ball.h"
#include "../entities/paddle.h"
#include "../entities/brick.h"
#include "../utils/collision.h"
#include "../utils/vector2d.h"

namespace breakout {

class PhysicsEngine {
public:
    Vector2D calculatePaddleReflection(const Vector2D& incomingVelocity, double hitPositionRatio) const;

    void resolveWallCollision(Ball& ball, const Rect& bounds) const;
    bool resolvePaddleCollision(Ball& ball, const Paddle& paddle) const;
    int resolveBrickCollisions(Ball& ball, std::vector<std::unique_ptr<Brick>>& bricks, double deltaTime, bool bigBallMode = false) const;
};

} // namespace breakout
