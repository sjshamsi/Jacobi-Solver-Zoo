#pragma once
#include <vector>

constexpr double DEFAULT_ALPHA{100.0};

// Fixed temperatures on the frame: the left column (column 0) rises from 0 at the top to alpha at
// the bottom, and the bottom row (the last rank's bottom halo row) falls from alpha on the left to 0
// on the right (the course's reference set-up). Local row k is padded global row rank_offset + k.
template <typename T>
void BF(std::vector<T> &field, const size_t nrows, const size_t ncols, const size_t rank_offset,
        const int rank, const int csize, const double alpha_val = DEFAULT_ALPHA) {
  const double edge = static_cast<double>(ncols - 1);   // N + 1

  for (size_t k = 0; k < nrows; k++)
    field[k * ncols] = static_cast<T>(alpha_val * static_cast<double>(rank_offset + k) / edge);

  if (rank + 1 == csize) {
    T *bottom = field.data() + (nrows - 1) * ncols;
    for (size_t c = 0; c < ncols; c++)
      bottom[c] = static_cast<T>(alpha_val * static_cast<double>(ncols - 1 - c) / edge);
  }
}
