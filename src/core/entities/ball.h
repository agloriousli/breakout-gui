#pragma once

#include "../utils/vector2d.h"
#include "../utils/collision.h"

namespace breakout {

struct BallState {
    Vector2D position;
    Vector2D velocity;
    double radius {6.0};
};

class Ball {
public:
    Ball() = default;
    explicit Ball(double radius) : radius_(radius) {}

    const Vector2D& position() const { return position_; }
    const Vector2D& velocity() const { return velocity_; }
    double radius() const { return radius_; }

    void setPosition(const Vector2D& pos) { position_ = pos; }
    void setVelocity(const Vector2D& vel) { velocity_ = vel; }
    void setRadius(double radius) { radius_ = radius; }

    Rect bounds() const { return { position_.x() - radius_, position_.y() - radius_, radius_ * 2.0, radius_ * 2.0 }; }

    void applyVelocity(double deltaTime) { position_ += velocity_ * deltaTime; }

    double speed() const { return velocity_.length(); }
    void setSpeedPreserveDirection(double speed);

    BallState state() const { return { position_, velocity_, radius_ }; }
    void restore(const BallState& state) {
        position_ = state.position;
        velocity_ = state.velocity;
        radius_ = state.radius;
    }

private:
    Vector2D position_ {0.0, 0.0};
    Vector2D velocity_ {0.0, 0.0};
    double radius_ {6.0};
};

} // namespace breakout
