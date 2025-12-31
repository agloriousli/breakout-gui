#include "vector2d.h"

namespace breakout {

Vector2D reflect(const Vector2D& incident, const Vector2D& normal) {
    Vector2D n = normal.normalized();
    double dotProd = incident.dot(n);
    return incident - 2.0 * dotProd * n;
}

} // namespace breakout
