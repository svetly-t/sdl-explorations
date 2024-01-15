#ifndef VECTOR_H_
#define VECTOR_H_

#include <cmath>

struct v2d {
  float x = 0;
  float y = 0;
  v2d() {}
  v2d(float _x, float _y) {
    x = _x;
    y = _y;
  }
  float Dot(const v2d &o) {
    return x * o.x + y * o.y;
  }
  float Cross(const v2d &o) {
    return x * o.y - y * o.x;
  }
  float SqrDistance(const v2d &o) {
    return (x - o.x) * (x - o.x) + (y - o.y) * (y - o.y);
  }
  float Distance(const v2d &o) {
    return sqrt(SqrDistance(o));
  }
  float SqrMagnitude() {
    return x * x + y * y;
  }
  float Magnitude() {
    return sqrt(x * x + y * y);
  }
  v2d Normalized() {
    float m = Magnitude();
    /* Can't normalize a zero-length vector */
    if (m == 0.0)
      return { 0.0, 0.0 };
    return v2d(x / m, y / m);
  }
  v2d Project(v2d axis) {
    if (axis.SqrMagnitude() == 0.0)
      return *this;
    axis = axis.Normalized();
    return axis * Dot(axis);
  }
  v2d ProjectTangent(v2d axis) {
    if (axis.SqrMagnitude() == 0.0)
      return *this;
    axis = axis.Normalized();
    axis = { axis.y, -axis.x };
    float proj = Dot(axis);
    return axis * proj;
  }
  v2d operator*(const float f) const {
    return v2d(x * f, y * f);
  }
  v2d operator*(const double d) const {
    return v2d(x * d, y * d);
  }
  v2d operator/(const float f) const {
    return v2d(x / f, y / f);
  }
  v2d operator/(const double d) const {
    return v2d(x / d, y / d);
  }
  v2d &operator+=(const v2d &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
  v2d &operator-=(const v2d &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }
  v2d &operator*=(const float f) {
    x *= f;
    y *= f;
    return *this;
  }
  v2d &operator/=(const float f) {
    x /= f;
    y /= f;
    return *this;
  }
  /* overload unary minus */
  v2d operator-() {
    return v2d(-x, -y);
  }
  /* overload subtract */
  v2d operator-(const v2d &rhs) {
    return v2d(x - rhs.x, y - rhs.y);
  }
  /* overload add */
  v2d operator+(const v2d &rhs) {
    return v2d(x + rhs.x, y + rhs.y);
  }
};

struct rect {
  float x;
  float y;
  float w;
  float h;
};

#endif