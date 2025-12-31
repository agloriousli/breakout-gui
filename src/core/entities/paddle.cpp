#include "paddle.h"

namespace breakout {

void Paddle::moveLeft(double deltaTime, double minX) {
    double newX = position_.x() - speed_ * deltaTime;
    if (newX < minX) newX = minX;
    position_.setX(newX);
}

void Paddle::moveRight(double deltaTime, double maxX) {
    double newX = position_.x() + speed_ * deltaTime;
    double limit = maxX - width_;
    if (newX > limit) newX = limit;
    position_.setX(newX);
}

} // namespace breakout
