/**
 * @file collision.cpp
 * @brief Collision detection utilities using Axis-Aligned Bounding Boxes (AABB).
 * 
 * This file provides collision detection functions for the game:
 * - Simple intersection tests between rectangles
 * - Swept AABB for continuous collision detection
 * - Vector utility functions
 * 
 * Swept AABB (Axis-Aligned Bounding Box) is used to detect collisions
 * for fast-moving objects like the ball. Unlike simple intersection tests,
 * swept AABB calculates the exact time of collision during movement,
 * preventing "tunneling" where fast objects pass through thin obstacles.
 */

#include "collision.h"

#include <cmath>
#include <iostream>
#include <limits>

using namespace std;

namespace breakout {

/**
 * @brief Test if two axis-aligned rectangles overlap.
 * 
 * This is a simple O(1) intersection test that checks if two rectangles
 * share any common area. Used for quick collision checks between game objects.
 * 
 * @param a First rectangle
 * @param b Second rectangle
 * @return true if rectangles overlap, false otherwise
 */
bool intersects(const Rect& a, const Rect& b) {
    return a.left() < b.right() && a.right() > b.left() && a.top() < b.bottom() && a.bottom() > b.top();
}

/**
 * @brief Perform swept AABB collision detection.
 * 
 * This function calculates if and when a moving rectangle will collide
 * with a static rectangle during a given time step. It uses the
 * Minkowski sum approach to expand the static rectangle and check
 * ray intersection.
 * 
 * Algorithm:
 * 1. Expand static rect by moving rect's dimensions (Minkowski sum)
 * 2. Treat moving rect as a point moving along velocity vector
 * 3. Calculate entry/exit times for each axis
 * 4. If the point enters the expanded box, a collision occurred
 * 5. Return the collision time and surface normal
 * 
 * @param movingRect The rectangle that is moving
 * @param velocity Movement velocity per second
 * @param staticRect The stationary rectangle to test against
 * @param deltaTime Time step duration in seconds
 * @return SweptAABBResult containing hit status, time, and collision normal
 */
SweptAABBResult sweptAABB(const Rect& movingRect, const Vector2D& velocity, const Rect& staticRect, double deltaTime) {
    SweptAABBResult result{};
    if (deltaTime <= 0.0) {
        return result;
    }

    // Expand the static rectangle by the moving rectangle's dimensions.
    // This is the Minkowski sum approach - if the moving rect's origin point
    // enters this expanded region, the rectangles are colliding.
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
        velocity.x() == 0.0 ? -numeric_limits<double>::infinity() : invEntry.x() / (velocity.x() * deltaTime),
        velocity.y() == 0.0 ? -numeric_limits<double>::infinity() : invEntry.y() / (velocity.y() * deltaTime)
    );

    Vector2D exitTime(
        velocity.x() == 0.0 ? numeric_limits<double>::infinity() : invExit.x() / (velocity.x() * deltaTime),
        velocity.y() == 0.0 ? numeric_limits<double>::infinity() : invExit.y() / (velocity.y() * deltaTime)
    );

    double entry = max(entryTime.x(), entryTime.y());
    double exit = min(exitTime.x(), exitTime.y());

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
