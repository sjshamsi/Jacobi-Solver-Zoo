#pragma once
#include <vector>
#include <functional>

inline constexpr double DEFAULT_ALPHA{100.0};
template<typename T> using bfunc_t = std::function<void(std::vector<T> &, size_t, double)>;

template<typename T> struct CMesh;
template<typename T> class PJacobiSolver;
template<typename T> class SJacobiSolver;
template<typename T> void print2DMatrix();

template<typename T> inline constexpr T FIELD_INIT_VAL{ static_cast<T>(0.5) };
template<typename T> inline constexpr T ZERO_VAL{ static_cast<T>(0.0) };


inline constexpr int DEFAULT_PRECISION{3};
inline constexpr size_t DEFAULT_WIDTH{8};

int numIntDigits(size_t n) {
  int digits = 1;
  while (n >= 10) {
    n /= 10;
    ++digits;
  }
  
  return digits;
}