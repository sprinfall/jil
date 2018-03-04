#ifndef BASE_MATH_UTIL_H_
#define BASE_MATH_UTIL_H_
#pragma once

#include <cmath>

namespace basemath {

const double PI = 3.14159265358979323846;
const double PI_1_2 = 1.57079632679489661923;  // 1/2 PI
const double PI_3_2 = 4.71238898038468985769;  // 3/2 PI

inline int Round(const double& d) {
  return static_cast<int>(std::floor(d + 0.5));
}

inline double RadianToDegree(double radians) {
  return radians * 180.0 / PI;
}

inline double DegreeToRadian(double degrees) {
  return degrees * PI / 180.0;
}

const double kRadian0 = 0.0;
const double kRadian90 = PI_1_2;
const double kRadian180 = PI;
const double kRadian270 = PI_3_2;

// Get the digit count of the number.
// 1 for number 0 ~ 9, 2 for number 10 ~ 99, etc.
int GetDigits(int number);

}  // namespace base

#endif  // BASE_MATH_UTIL_H_
