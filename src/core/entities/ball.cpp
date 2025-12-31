#include "ball.h"

namespace breakout {

void Ball::setSpeedPreserveDirection(double speed) {
    Vector2D dir = velocity_.normalized();
    velocity_ = dir * speed;
}

} // namespace breakout
