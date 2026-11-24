#pragma once

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <mpi.h>
#include <omp.h>

#include "Mesh.hpp"

#include "Print2DMatrix.hpp"
#include "MPI_Timer.hpp"

template <typename T>
class PJacobi {
public:
  explicit PJacobi(Mesh<T> &mesh_instance, MPITiming::Timekeeper &timekeeper) : mesh(mesh_instance), tk(timekeeper) {}
  void solve(size_t max_iters);

private:
  Mesh<T> &mesh;
  MPITiming::Timekeeper &tk;
  void process_rows(const size_t i_0, const size_t i_f);
};


template<typename T>
void PJacobi<T>::process_rows(const size_t i_0, const size_t i_f) {
  #pragma omp parallel for schedule(static)
  for (size_t i = i_0; i < i_f; i++) {
    T* __restrict rowB = mesh.fieldB.data() + i * mesh.ncols;
    const T* __restrict rowA_above = mesh.fieldA.data() + (i - 1) * mesh.ncols;
    const T* __restrict rowA       = mesh.fieldA.data() + i * mesh.ncols;
    const T* __restrict rowA_below = mesh.fieldA.data() + (i + 1) * mesh.ncols;
    
    #pragma omp simd
    for (size_t j = 1; j < mesh.ncols - 1; j++) {
      rowB[j] = 0.25 * (rowA_above[j] + rowA_below[j] + rowA[j - 1] + rowA[j + 1]);
    }
  }
}


template <typename T>
void PJacobi<T>::solve(size_t max_iters) {
  for (size_t i = 0; i < max_iters; i++) {
    MPITiming::Stopwatch sw1(tk);
    sw1.start("process_outer");
    process_rows(1, 2);
    sw1.stop();

    MPITiming::Stopwatch sw2(tk);
    sw2.start("process_body");
    process_rows(mesh.nrows - 2, mesh.nrows - 1);
    sw2.stop();

    mesh.exchange_halos(mesh.fieldB);

    process_rows(2, mesh.nrows - 2);
    MPI_Waitall(4, mesh.exchange_rqs.data(), MPI_STATUSES_IGNORE);
    std::swap(mesh.fieldA, mesh.fieldB);
  }
}