#pragma once

#include <cmath>

namespace breakout {

class Vector2D {
public:
    Vector2D() = default;
    Vector2D(double xVal, double yVal) : x_(xVal), y_(yVal) {}

    double x() const { return x_; }
    double y() const { return y_; }

    void setX(double xVal) { x_ = xVal; }
    void setY(double yVal) { y_ = yVal; }

    double length() const { return std::sqrt(x_ * x_ + y_ * y_); }

    Vector2D normalized() const {
        double len = length();
        if (len == 0.0) {
            return Vector2D(0.0, 0.0);
        }
        return Vector2D(x_ / len, y_ / len);
    }

    double dot(const Vector2D& other) const { return x_ * other.x_ + y_ * other.y_; }

    Vector2D operator+(const Vector2D& rhs) const { return Vector2D(x_ + rhs.x_, y_ + rhs.y_); }
    Vector2D operator-(const Vector2D& rhs) const { return Vector2D(x_ - rhs.x_, y_ - rhs.y_); }
    Vector2D operator*(double scalar) const { return Vector2D(x_ * scalar, y_ * scalar); }
    Vector2D operator/(double scalar) const { return Vector2D(x_ / scalar, y_ / scalar); }

    Vector2D& operator+=(const Vector2D& rhs) {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    Vector2D& operator-=(const Vector2D& rhs) {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    Vector2D& operator*=(double scalar) {
        x_ *= scalar;
        y_ *= scalar;
        return *this;
    }

private:
    double x_ {0.0};
    double y_ {0.0};
};

inline Vector2D operator*(double scalar, const Vector2D& v) { return v * scalar; }

Vector2D reflect(const Vector2D& incident, const Vector2D& normal);

} // namespace breakout
