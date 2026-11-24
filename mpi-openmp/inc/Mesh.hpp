#pragma once

#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <numeric>
#include <functional>
#include <type_traits>

#include "traits.hpp"

#include "Print2DMatrix.hpp"
#include "MPI_Timer.hpp"


inline constexpr int DEFAULT_ROOT{}, DEFAULT_WIDTH{-1}, DEFAULT_PRECISION{3};
template<typename T> class PJacobi;
template<typename T> using bfunc_t = std::function<void(std::vector<T> &, const size_t, const size_t, const size_t,
                                                        const int, const int, const double)>;
template<typename T> inline constexpr T FIELD_INIT_VAL{static_cast<T>(0.5)};


//////////////////////////////
// # int - size_t conversion functions
//////////////////////////////
static int toINT(const size_t v) {
  if (v > static_cast<size_t>(std::numeric_limits<int>::max())) {
    std::cerr << "Error: size_t value " << v << " exceeds the maximum value of int ("
              << std::numeric_limits<int>::max() << ")." << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return static_cast<int>(v);
}

static size_t toST(const int v) {
  if (v < 0) {
    std::cerr << "Error: int value " << v << " is negative and cannot be converted to size_t!";
    std::exit(EXIT_FAILURE);
  }
  return static_cast<size_t>(v);
}

//////////////////////////////
// # HELPERS to get MPI Types
//////////////////////////////
template <typename T>
MPI_Datatype mpi_type() {
  if constexpr      (std::is_same_v<T, char>)               return MPI_CHAR;
  else if constexpr (std::is_same_v<T, signed char>)        return MPI_SIGNED_CHAR;
  else if constexpr (std::is_same_v<T, unsigned char>)      return MPI_UNSIGNED_CHAR;
  else if constexpr (std::is_same_v<T, short>)              return MPI_SHORT;
  else if constexpr (std::is_same_v<T, unsigned short>)     return MPI_UNSIGNED_SHORT;
  else if constexpr (std::is_same_v<T, int>)                return MPI_INT;
  else if constexpr (std::is_same_v<T, unsigned int>)       return MPI_UNSIGNED;
  else if constexpr (std::is_same_v<T, long>)               return MPI_LONG;
  else if constexpr (std::is_same_v<T, unsigned long>)      return MPI_UNSIGNED_LONG;
  else if constexpr (std::is_same_v<T, long long>)          return MPI_LONG_LONG;
  else if constexpr (std::is_same_v<T, unsigned long long>) return MPI_UNSIGNED_LONG_LONG;
  else if constexpr (std::is_same_v<T, float>)              return MPI_FLOAT;
  else if constexpr (std::is_same_v<T, double>)             return MPI_DOUBLE;
  else if constexpr (std::is_same_v<T, long double>)        return MPI_LONG_DOUBLE;
#ifdef MPI_C_BOOL
  else if constexpr (std::is_same_v<T, bool>)               return MPI_C_BOOL;
#endif
  else static_assert(always_false_v<T>, "mpi_type<T>: unsupported C++ type for MPI_Datatype");
}


//////////////////////////////
// # CLASS: Mesh
//////////////////////////////
template<typename T>
class Mesh {
public:
  size_t N;
  MPITiming::Timekeeper &tk;
  int root;
  MPI_Comm comm;
  int rank, csize;
  size_t rows_remaining, nrows, offset, ncols;

  std::vector<size_t> all_nrows;
  std::vector<size_t> all_offsets;
  void calc_nrows_offsets();

  // Exchange halos
  mutable std::vector<MPI_Request> exchange_rqs;
  void exchange_halos(std::vector<T> &field);

  // Things for stdio printing
  mutable MPI_Request stdio_req{MPI_REQUEST_NULL};
  MPI_Datatype send_t{MPI_DATATYPE_NULL};
  std::vector<int> recvcounts, displs;

  // Data and Misc.
  std::vector<T> fieldA, fieldB;
  void init_mesh();
  void post_error(std::string_view msg) const;

  // Constructor and stdio methods
  explicit Mesh(const size_t field_dim, bfunc_t<T> boundary_function, MPITiming::Timekeeper &timekeeper,
                const int root_rank = DEFAULT_ROOT, const MPI_Comm mpi_comm = MPI_COMM_WORLD);

  std::ostream & print_boxed(std::ostream &os = std::cout,
                             int width = DEFAULT_WIDTH, int precision = DEFAULT_PRECISION) const;
  std::ostream & print(std::ostream &os = std::cout, std::string_view delim = ",",
                       int width = DEFAULT_WIDTH, int precision = DEFAULT_PRECISION) const;
  void copy_matrix(std::vector<T> &external_buff);
};

//////////////////////////////
// # CLASS: Mesh: Constructor
//////////////////////////////
template<typename T>
Mesh<T>::Mesh(const size_t field_dim, bfunc_t<T> boundary_function, MPITiming::Timekeeper &timekeeper,
              const int root_rank, const MPI_Comm mpi_comm) :
  N(field_dim), tk(timekeeper), root(root_rank), comm(mpi_comm) {
  static_assert(std::is_arithmetic_v<T>, "CLASS CMatrix: Matrix type must be numeric!\n");
  
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &csize);
  assert(N  >= toST(csize));

  rows_remaining = N % csize;
  
  calc_nrows_offsets();
  nrows = all_nrows[rank];
  offset = all_offsets[rank];
  ncols = N + 2;

  exchange_rqs.assign(4, MPI_REQUEST_NULL);

  MPI_Type_vector(toINT(nrows - 2), toINT(N), toINT(ncols), mpi_type<T>(), &send_t);
  MPI_Type_commit(&send_t);
  
  recvcounts.reserve(csize);
  displs.reserve(csize);
  for (int i = 0; i < csize; i++) {
    recvcounts.push_back( toINT(N * (all_nrows[i] - 2)) );
    displs.push_back( toINT(N * all_offsets[i]) );
  }

  init_mesh();
  boundary_function(fieldA, nrows, ncols, offset, rank, csize, 100.0);
  fieldB = fieldA;  // both fields need the same fixed frame, since they swap every sweep
}


template<typename T>
void Mesh<T>::init_mesh() {
  MPITiming::Stopwatch sw(tk);
  sw.start("meshinit");
  size_t k0{}, kf{nrows};
  fieldA.assign(nrows * ncols, FIELD_INIT_VAL<T>);
  fieldB.assign(nrows * ncols, FIELD_INIT_VAL<T>);

  if (rank == root) {
    std::fill(fieldA.begin(), fieldA.begin() + ncols, T{});
    std::fill(fieldB.begin(), fieldB.begin() + ncols, T{});
    k0++;
  }

  if (rank == csize - 1) {
    std::fill(fieldA.begin() + (nrows - 1) * ncols, fieldA.begin() + nrows * ncols, T{});
    std::fill(fieldB.begin() + (nrows - 1) * ncols, fieldB.begin() + nrows * ncols, T{});
    kf--;
  }
  
  for (size_t k = k0; k < kf; k++) {
    fieldA[k * ncols] = T{};
    fieldB[k * ncols] = T{};
    fieldA[k * ncols + (N + 1)] = T{};
    fieldB[k * ncols + (N + 1)] = T{};
  }
  sw.stop();
}


//////////////////////////////
// # CLASS: Mesh: STD IO Methods
//////////////////////////////
template <typename T>
std::ostream & Mesh<T>::print_boxed(std::ostream &os, int width, int precision) const {
  if (rank) {
    MPI_Wait(&stdio_req, MPI_STATUS_IGNORE);
    MPI_Isend(fieldA.data() + ncols + 1, 1, send_t, root, rank, comm, &stdio_req);
    return os;
  }

  if (width == DEFAULT_WIDTH) width = 2 + MatrixPrint::optimal_width(fieldA, precision);
  std::vector<T> buff1(N * (nrows - 2)), buff2(N * (nrows - 2));

  for (size_t i = 1; i < nrows - 1; i++)
    std::memcpy(buff2.data() + N * (i - 1), fieldA.data() + (i * ncols) + 1, N * sizeof(T));

  for (int i = 1; i < csize; i++) {
    MPI_Irecv(buff1.data(), toINT(N * (all_nrows[i] - 2)), mpi_type<T>(), i, i, comm, &stdio_req);
    MatrixPrint::print_boxed_2D(buff2, all_nrows[i - 1] - 2, N, os, false, width, precision);
    MPI_Wait(&stdio_req, MPI_STATUS_IGNORE);
    std::swap(buff1, buff2);
  }

  MatrixPrint::print_boxed_2D(buff2, all_nrows[csize - 1] - 2, N, os, true, width, precision);
  return os;
}


template <typename T>
std::ostream & Mesh<T>::print(std::ostream &os, std::string_view delim, int width, int precision) const {
  if (rank) {
    MPI_Wait(&stdio_req, MPI_STATUS_IGNORE);
    MPI_Isend(fieldA.data() + ncols + 1, 1, send_t, root, rank, comm, &stdio_req);
    return os;
  }

  if (width == DEFAULT_WIDTH) width = 1 + MatrixPrint::optimal_width(fieldA, precision);
  std::vector<T> buff1(N * (nrows - 2)), buff2(N * (nrows - 2));

  for (size_t i = 1; i < nrows - 1; i++)
    std::memcpy(buff2.data() + N * (i - 1), fieldA.data() + (i * ncols) + 1, N * sizeof(T));

  for (int i = 1; i < csize; i++) {
    MPI_Irecv(buff1.data(), toINT(N * (all_nrows[i] - 2)), mpi_type<T>(), i, i, comm, &stdio_req);
    MatrixPrint::print_2D(buff2, all_nrows[i - 1] - 2, N, os, delim, width, precision);
    MPI_Wait(&stdio_req, MPI_STATUS_IGNORE);
    std::swap(buff1, buff2);
  }
  
  MatrixPrint::print_2D(buff2, all_nrows[csize - 1] - 2, N, os, delim, width, precision);
  return os;
}


template <typename T>
void Mesh<T>::copy_matrix(std::vector<T> &external_buff) {
  if (rank == root) external_buff.resize(N * N);

  MPI_Gatherv(fieldA.data() + ncols + 1, 1, send_t,
              external_buff.data(), recvcounts.data(), displs.data(), mpi_type<T>(), root, comm);
}

//////////////////////////////
// # CLASS: Mesh: Misc.
//////////////////////////////
template <typename T>
void Mesh<T>::calc_nrows_offsets() {
  size_t base_nrows{N / csize};
  all_nrows.reserve(csize);
  all_offsets.reserve(csize);

  for (size_t i = 0; i < rows_remaining; i++) {
    all_nrows.push_back(base_nrows + 3);
    all_offsets.push_back(i * (base_nrows + 1));
  }

  for (size_t i = rows_remaining; i < toST(csize); i++) {
    all_nrows.push_back(base_nrows + 2);
    all_offsets.push_back(rows_remaining * (base_nrows + 1) + base_nrows * (i - rows_remaining));
  }   
}


template <typename T>
void Mesh<T>::exchange_halos(std::vector<T> &field) {
  MPITiming::Stopwatch sw(tk);
  sw.start("halofn");

  if (rank + 1 < csize) MPI_Isend(field.data() + 1 + ncols * (nrows - 2), toINT(N), mpi_type<T>(), rank + 1, rank, comm, &exchange_rqs[0]);
  if (rank > 0) MPI_Irecv(field.data() + 1, toINT(N), mpi_type<T>(), rank - 1, rank - 1, comm, &exchange_rqs[1]);
  
  if (rank > 0) MPI_Isend(field.data() + 1 + ncols, toINT(N), mpi_type<T>(), rank - 1, rank, comm, &exchange_rqs[2]);
  if (rank + 1 < csize) MPI_Irecv(field.data() + 1 + ncols * (nrows - 1), toINT(N), mpi_type<T>(), rank + 1, rank + 1, comm, &exchange_rqs[3]);
  
  sw.stop();
}