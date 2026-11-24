#pragma once
#include <vector>

// Fixed temperatures on the frame of the (N+2) x (N+2) field: the left column rises from 0 at the
// top to alpha at the bottom, and the bottom row falls from alpha on the left to 0 on the right
// (the course's reference set-up). The top row and right column stay at 0.
template <typename T>
void BF(std::vector<T> &field, size_t N, double alpha_val = DEFAULT_ALPHA) {
  const size_t M = N + 2;

  for (size_t r = 1; r <= N + 1; r++) {
    const T field_val = static_cast<T>( alpha_val * ( static_cast<double>(r) / static_cast<double>(N + 1) ) );

    field[r * M] = field_val;                       // left column, row r
    field[(N + 1) * M + (N + 1 - r)] = field_val;   // bottom row, column N + 1 - r
  }
}
