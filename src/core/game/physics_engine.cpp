#include "physics_engine.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

namespace breakout {

namespace {

inline double clamp(double value, double minVal, double maxVal) {
    return std::max(minVal, std::min(value, maxVal));
}

}

Vector2D PhysicsEngine::calculatePaddleReflection(const Vector2D& incomingVelocity, double hitPositionRatio) const {
    const double theta0 = M_PI / 2.0;
    const double k = M_PI / 3.0;

    // Negative hitPositionRatio (left side) should yield exit angles > 90° (leftward).
    double exitAngle = theta0 - k * hitPositionRatio;
    exitAngle = clamp(exitAngle, M_PI / 6.0, 5.0 * M_PI / 6.0);

    double speed = incomingVelocity.length();
    double vx = speed * std::cos(exitAngle);
    double vy = -speed * std::sin(exitAngle);
    return { vx, vy };
}

void PhysicsEngine::resolveWallCollision(Ball& ball, const Rect& bounds) const {
    Rect b = ball.bounds();
    Vector2D vel = ball.velocity();

    if (b.left() < bounds.left()) {
        ball.setPosition({ bounds.left() + ball.radius(), ball.position().y() });
        vel.setX(-vel.x());
    } else if (b.right() > bounds.right()) {
        ball.setPosition({ bounds.right() - ball.radius(), ball.position().y() });
        vel.setX(-vel.x());
    }

    if (b.top() < bounds.top()) {
        ball.setPosition({ ball.position().x(), bounds.top() + ball.radius() });
        vel.setY(-vel.y());
        
        // Prevent perfectly vertical bounces by adding small horizontal component
        if (std::abs(vel.x()) < 0.1) {
            double speed = vel.length();
            double minAngle = 0.1; // Small angle to prevent vertical bounce
            vel.setX(speed * minAngle * (vel.x() >= 0 ? 1.0 : -1.0));
            vel.setY(-std::sqrt(speed * speed - vel.x() * vel.x()));
        }
    }

    ball.setVelocity(vel);
}

bool PhysicsEngine::resolvePaddleCollision(Ball& ball, const Paddle& paddle) const {
    if (ball.velocity().y() <= 0.0) {
        return false;
    }

    if (!intersects(ball.bounds(), paddle.bounds())) {
        return false;
    }

    double paddleCenter = paddle.position().x() + paddle.width() * 0.5;
    double hitRatio = (ball.position().x() - paddleCenter) / (paddle.width() * 0.5);
    hitRatio = clamp(hitRatio, -1.0, 1.0);

    Vector2D newVelocity = calculatePaddleReflection(ball.velocity(), hitRatio);
    ball.setVelocity(newVelocity);
    ball.setPosition({ ball.position().x(), paddle.position().y() - ball.radius() });
    
    return true;
}

int PhysicsEngine::resolveBrickCollisions(Ball& ball, std::vector<std::unique_ptr<Brick>>& bricks, double deltaTime, bool bigBallMode) const {
    int destroyed = 0;
    double remainingTime = 1.0;
    Vector2D velocity = ball.velocity();

    for (int iteration = 0; iteration < 3 && remainingTime > 0.0; ++iteration) {
        double earliest = 1.0;
        double hitDistance = std::numeric_limits<double>::max();
        size_t hitIndex = bricks.size();
        Vector2D hitNormal {0.0, 0.0};
        
        // Epsilon for comparing collision times (consider equal if within this threshold)
        const double TIME_EPSILON = 0.0001;

        for (size_t i = 0; i < bricks.size(); ++i) {
            // Skip destroyed bricks
            if (bricks[i]->isDestroyed()) {
                continue;
            }
            
            const Rect& box = bricks[i]->bounds();
            SweptAABBResult result = sweptAABB(ball.bounds(), velocity, box, deltaTime * remainingTime);
            
            if (result.hit) {
                // Calculate distance from brick center to ball center for tie-breaking
                double brickCenterX = box.x + box.width * 0.5;
                double brickCenterY = box.y + box.height * 0.5;
                double dx = brickCenterX - ball.position().x();
                double dy = brickCenterY - ball.position().y();
                double distance = std::sqrt(dx * dx + dy * dy);
                
                // Primary criterion: collision time (significantly earlier wins)
                if (result.time < earliest - TIME_EPSILON) {
                    earliest = result.time;
                    hitDistance = distance;
                    hitIndex = i;
                    hitNormal = result.normal;
                }
                // Tie-breaker: if times are approximately equal, pick closer brick
                else if (std::abs(result.time - earliest) <= TIME_EPSILON && distance < hitDistance) {
                    earliest = result.time;
                    hitDistance = distance;
                    hitIndex = i;
                    hitNormal = result.normal;
                }
            }
        }

        if (hitIndex == bricks.size()) {
            break;
        }

        double travelTime = earliest * deltaTime * remainingTime;
        ball.setPosition(ball.position() + velocity * travelTime);
        velocity = reflect(velocity, hitNormal);
        // Nudge ball out along the collision normal to avoid sticking where a brick was removed.
        ball.setPosition(ball.position() + hitNormal * (ball.radius() * 0.5));
        ball.setVelocity(velocity);
        
        bool brickWillBeDestroyed = bricks[hitIndex]->applyHit();

        if (brickWillBeDestroyed) {
            ++destroyed;
            
            // Big ball mode: destroy nearby bricks within radius
            if (bigBallMode) {
                Vector2D ballCenter = ball.position();
                double destructionRadius = ball.radius() * 2.5;  // Destroy bricks within 2.5x ball radius
                
                for (size_t i = 0; i < bricks.size(); ++i) {
                    // Skip already destroyed bricks
                    if (bricks[i]->isDestroyed()) {
                        continue;
                    }
                    
                    const Rect& brickBox = bricks[i]->bounds();
                    double brickCenterX = brickBox.x + brickBox.width * 0.5;
                    double brickCenterY = brickBox.y + brickBox.height * 0.5;
                    double dx = brickCenterX - ballCenter.x();
                    double dy = brickCenterY - ballCenter.y();
                    double distance = std::sqrt(dx * dx + dy * dy);
                    
                    if (distance <= destructionRadius) {
                        if (bricks[i]->applyHit()) {  // Returns true if destroyed
                            ++destroyed;
                        }
                    }
                }
            }
        }

        remainingTime *= (1.0 - earliest);
    }

    if (remainingTime > 0.0) {
        ball.setPosition(ball.position() + velocity * (deltaTime * remainingTime));
    }

    return destroyed;
}

} // namespace breakout
