#pragma once

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

#include "constants.hpp"


template<typename T>
std::string formatCell(T value, size_t width, int precision) {
  std::ostringstream oss;
  if constexpr (std::is_floating_point_v<T>) { // Floating-point: fixed precision
    oss << std::fixed << std::setprecision(precision) << value;
  
  } else {
    oss << value;
  }
  
  std::string str = oss.str();
  
  if (width < str.size()) return str;  // too long for the box
    
  size_t pad = width - str.size();
  size_t left = pad / 2;
  size_t right = pad - left;

  return std::string(left, ' ') + str + std::string(right, ' ');
}


template<typename T>
void print2DMatrix(const std::vector<T> &M, size_t nrows, size_t ncols,
  size_t width = DEFAULT_WIDTH, int precision = DEFAULT_PRECISION, std::ostream &os = std::cout) {
  
  std::string row_seperator = "+";

  // Build a row separator like +----------+----------+...
  for (size_t j = 0; j < ncols; j++) row_seperator += std::string(width, '-') + "+";
  
  os << "\n" << row_seperator << "\n";
  for (size_t i = 0; i < nrows; i++) {
    
    for (size_t j = 0; j < ncols; j++) {
      os << "|";
      os << formatCell(M[i * nrows + j], width, precision);
    }
    
    os << "|";
    os << "\n" << row_seperator << "\n";
  }
    
  os << "\n";
}

/* TEST FOR FUNCTIONS

int main() {
    std::vector<double> mat = { 3.14159, 2.71828, 1.41421, 0.57721, -1.0, 42.0};
    print2DMatrix<double>(mat, size_t{3}, size_t{2});

    // std::vector<int> mat = { 3, 2, 1, 0, -1, 42};
    // print2DMatrix<int>(mat, size_t{3}, size_t{2}, 10, 2);

    return 0;
}

*/