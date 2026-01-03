#pragma once

#include <algorithm>
#include <limits>

#include "vector2d.h"

namespace breakout {

struct Rect {
    double x {0.0};
    double y {0.0};
    double width {0.0};
    double height {0.0};

    double left() const { return x; }
    double right() const { return x + width; }
    double top() const { return y; }
    double bottom() const { return y + height; }
    Vector2D center() const { return { x + width * 0.5, y + height * 0.5 }; }
};

struct SweptAABBResult {
    bool hit {false};
    double time {1.0};
    Vector2D normal {0.0, 0.0};
};

bool intersects(const Rect& a, const Rect& b);
SweptAABBResult sweptAABB(const Rect& movingRect, const Vector2D& velocity, const Rect& staticRect, double deltaTime);
Vector2D clampVector(const Vector2D& value, double maxLength);

} // namespace breakout
