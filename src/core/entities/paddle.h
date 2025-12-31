#pragma once

#include "../utils/vector2d.h"
#include "../utils/collision.h"

namespace breakout {

struct PaddleState {
    Vector2D position;
    double width {80.0};
    double height {16.0};
};

class Paddle {
public:
    Paddle() = default;
    Paddle(double width, double height, double speed) : width_(width), height_(height), speed_(speed) {}

    const Vector2D& position() const { return position_; }
    double width() const { return width_; }
    double height() const { return height_; }
    double speed() const { return speed_; }

    void setPosition(const Vector2D& pos) { position_ = pos; }
    void setSpeed(double speed) { speed_ = speed; }
    void setSize(double width, double height) {
        width_ = width;
        height_ = height;
    }

    Rect bounds() const { return { position_.x(), position_.y(), width_, height_ }; }

    void moveLeft(double deltaTime, double minX);
    void moveRight(double deltaTime, double maxX);

    PaddleState state() const { return { position_, width_, height_ }; }
    void restore(const PaddleState& state) {
        position_ = state.position;
        width_ = state.width;
        height_ = state.height;
    }

private:
    Vector2D position_ {0.0, 0.0};
    double width_ {80.0};
    double height_ {16.0};
    double speed_ {280.0};
};

} // namespace breakout
