#include "collision.h"

#include <cmath>
#include <iostream>

namespace breakout {

bool intersects(const Rect& a, const Rect& b) {
    return a.left() < b.right() && a.right() > b.left() && a.top() < b.bottom() && a.bottom() > b.top();
}

SweptAABBResult sweptAABB(const Rect& movingRect, const Vector2D& velocity, const Rect& staticRect, double deltaTime) {
    SweptAABBResult result{};
    if (deltaTime <= 0.0) {
        return result;
    }

    Rect expanded { staticRect.x - movingRect.width, staticRect.y - movingRect.height,
                    staticRect.width + movingRect.width, staticRect.height + movingRect.height };

    // When velocity is zero on an axis, check if we're already overlapping on that axis
    // If not overlapping, no collision is possible
    if (velocity.x() == 0.0) {
        if (movingRect.x < expanded.left() || movingRect.x > expanded.right()) {
            return result;  // No overlap on x-axis and no x movement = no collision
        }
    }
    if (velocity.y() == 0.0) {
        if (movingRect.y < expanded.top() || movingRect.y > expanded.bottom()) {
            return result;  // No overlap on y-axis and no y movement = no collision
        }
    }

    Vector2D invEntry;
    Vector2D invExit;

    if (velocity.x() > 0.0) {
        invEntry.setX(expanded.left() - movingRect.x);
        invExit.setX(expanded.right() - movingRect.x);
    } else {
        invEntry.setX(expanded.right() - movingRect.x);
        invExit.setX(expanded.left() - movingRect.x);
    }

    if (velocity.y() > 0.0) {
        invEntry.setY(expanded.top() - movingRect.y);
        invExit.setY(expanded.bottom() - movingRect.y);
    } else {
        invEntry.setY(expanded.bottom() - movingRect.y);
        invExit.setY(expanded.top() - movingRect.y);
    }

    Vector2D entryTime(
        velocity.x() == 0.0 ? -std::numeric_limits<double>::infinity() : invEntry.x() / (velocity.x() * deltaTime),
        velocity.y() == 0.0 ? -std::numeric_limits<double>::infinity() : invEntry.y() / (velocity.y() * deltaTime)
    );

    Vector2D exitTime(
        velocity.x() == 0.0 ? std::numeric_limits<double>::infinity() : invExit.x() / (velocity.x() * deltaTime),
        velocity.y() == 0.0 ? std::numeric_limits<double>::infinity() : invExit.y() / (velocity.y() * deltaTime)
    );

    double entry = std::max(entryTime.x(), entryTime.y());
    double exit = std::min(exitTime.x(), exitTime.y());

    if (entry > exit || entry < 0.0 || entry > 1.0) {
        return result;
    }

    result.hit = true;
    result.time = entry;

    if (entryTime.x() > entryTime.y()) {
        result.normal = velocity.x() < 0.0 ? Vector2D(1.0, 0.0) : Vector2D(-1.0, 0.0);
    } else {
        result.normal = velocity.y() < 0.0 ? Vector2D(0.0, 1.0) : Vector2D(0.0, -1.0);
    }

    return result;
}

Vector2D clampVector(const Vector2D& value, double maxLength) {
    double len = value.length();
    if (len <= maxLength || len == 0.0) {
        return value;
    }
    return value.normalized() * maxLength;
}

} // namespace breakout
