#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
#include <numeric>

#include "constants.hpp"


template<typename T>
struct CMesh {
  explicit CMesh(size_t field_dim, bfunc_t<T> boundary_function);
  
  std::vector<T> fieldA, fieldB;
  size_t N, M, fsize;
  
  void print(size_t width = DEFAULT_WIDTH, int precision = DEFAULT_PRECISION, std::ostream &os = std::cout);
  void print_to_file(const std::string &filename);
  T get_total() { return std::accumulate(fieldA.begin(), fieldA.end(), ZERO_VAL<T>); }
};


template<typename T>
CMesh<T>::CMesh(size_t field_dim, bfunc_t<T> boundary_function)
  : N(field_dim), M(field_dim + 2), fsize( (field_dim + 2) * (field_dim + 2) ) {

  fieldA.assign(fsize, FIELD_INIT_VAL<T>);
  fieldB.assign(fsize, FIELD_INIT_VAL<T>);

  for (size_t k = 0; k < fsize; k++) {
    fieldA[k] = ZERO_VAL<T>;
    fieldB[k] = ZERO_VAL<T>;

    k += N + 1;
    fieldA[k] = ZERO_VAL<T>;
    fieldB[k] = ZERO_VAL<T>;
  }
  
  std::fill(fieldA.begin() + 1, fieldA.begin() + N + 1, ZERO_VAL<T>);
  std::fill(fieldA.begin() + 1 + ( (N + 1) * (N + 2) ), fieldA.begin() + (N + 1) * (N + 3), ZERO_VAL<T>);
  
  boundary_function(fieldA, field_dim, DEFAULT_ALPHA);
  fieldB = fieldA;  // both fields need the same fixed frame, since they swap every sweep
}


template<typename T>
void CMesh<T>::print(size_t width, int precision, std::ostream &os) {
  print2DMatrix<T>(fieldA, M, M, width, precision, os);
}


template<typename T>
void CMesh<T>::print_to_file(const std::string &filename) {
  std::ofstream outfile (filename);
  
  for (size_t i = 0; i < M; i++) {
    for (size_t j = 0; j < M; j++) {
        outfile << fieldA[i * (M) + j] << " ";
    }
    outfile << std::endl;
}
  outfile.close();
}