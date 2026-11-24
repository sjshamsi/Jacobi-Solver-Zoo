#pragma once

#include <iostream>
#include <fstream>
#include <iomanip>
#include <mpi.h>
#include <openacc.h>
#include <nvToolsExt.h>
#include <limits>

static constexpr int ROOT{}, DEFAULT_PRECISION{6}, DEFAULT_WIDTH{8};
static constexpr double FIELD_INIT_VAL{0.5}, BF_ALPHA{100.0};


// ------------------------------------------------------------------------
// int <--> size_t conversion helpers
// ------------------------------------------------------------------------
static int asINT(const size_t v) {
  if (v > static_cast<size_t>(std::numeric_limits<int>::max())) {
    std::cerr << "Error: size_t value " << v << " exceeds the maximum value of int ("
              << std::numeric_limits<int>::max() << ")." << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return static_cast<int>(v);
}

static size_t asST(const int v) {
  if (v < 0) {
    std::cerr << "Error: int value " << v << " is negative and cannot be converted to size_t!";
    std::exit(EXIT_FAILURE);
  }
  return static_cast<size_t>(v);
}


// ------------------------------------------------------------------------
// CLASS DECLARATION
// ------------------------------------------------------------------------
class Mesh {
public:
  Mesh(size_t field_dim, MPI_Comm mpicomm=MPI_COMM_WORLD);
  ~Mesh();
  
  size_t N;
  MPI_Comm comm;
  int rank, csize, n_devices, device_id;
  size_t nrows, ncols, msize;

  double *fieldA{nullptr}, *fieldB{nullptr}, *print_buff{nullptr};
  void boundary_function();

  MPI_Request exchange_rqs[4] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL};
  void exchange_B_halos();

  void swapAB();
  
  void iter();
  void solve(size_t max_iters, size_t write_intvl, std::string_view filepath_prefix);
  void write_step(std::string_view filepath_prefix, size_t iter_idx, size_t max_iters);

  MPI_Datatype col_t;
  std::ostream & print(std::ostream &os=std::cout);
  std::ostream & print_rankAB(std::ostream &os=std::cout);
};


// ------------------------------------------------------------------------
// CONSTRUCTOR and DESTRUCTOR
// ------------------------------------------------------------------------
Mesh::Mesh(size_t field_dim, MPI_Comm mpicomm) : N(field_dim), comm(mpicomm) {
  MPI_Comm_size(comm, &csize);
  MPI_Comm_rank(comm, &rank);

  n_devices = acc_get_num_devices(acc_device_nvidia);
  device_id = rank % n_devices;
  acc_set_device_num(device_id, acc_device_nvidia);

  if (N % csize) {
    if (rank == ROOT) std::cerr << "Field dimention must be a multiple of the number of processes.\n";
    std::exit(EXIT_FAILURE);
  }

  nrows = (N / csize) + 2;
  ncols = N + 2;
  msize = nrows * ncols;
  
  MPI_Type_vector(asINT(nrows - 2), asINT(N), asINT(ncols), MPI_DOUBLE, &col_t);
  MPI_Type_commit(&col_t);

  fieldA = (double*) malloc(msize * sizeof(double));
  fieldB = (double*) malloc(msize * sizeof(double));
  if (rank == ROOT)
    print_buff = (double*) malloc(N * N * sizeof(double));

  for (size_t i = 0; i < msize; i++) fieldA[i] = FIELD_INIT_VAL;

  size_t k0{}, kf{nrows};

  if (rank == ROOT) {
    for (size_t i = 0; i < ncols; i++) fieldA[i] = 0.0;
    k0++;
  }

  if (rank == csize - 1) {
    for (size_t i = ncols * (nrows - 1); i < msize; i++) fieldA[i] = 0.0;
    kf--;
  }

  for (size_t k = k0; k < kf; k++) {
    fieldA[k * ncols] = 0.0;
    fieldA[(k * ncols) + ncols - 1] = 0.0;
  }

  boundary_function();

  for (size_t i = 0; i < msize; i++) fieldB[i] = fieldA[i];

  #pragma acc enter data copyin(fieldA[0:msize], fieldB[0:msize])
  
  return;
}


Mesh::~Mesh() {
  #pragma acc exit data delete(fieldA[0:msize], fieldB[0:msize])

  free(fieldA);
  free(fieldB);
  if (rank == ROOT)
    free(print_buff);
}


// ------------------------------------------------------------------------
// Boundary Function
// ------------------------------------------------------------------------
// Fixed temperatures on the frame: column 0 rises from 0 at the top to BF_ALPHA at the bottom, and
// the bottom row (the last rank's bottom halo row) falls from BF_ALPHA on the left to 0 on the right
// (the course's reference set-up). Local row k is padded global row rank_offset + k.
void Mesh::boundary_function() {
  const size_t rank_offset{asST(rank) * (nrows - 2)};
  const double edge{static_cast<double>(N + 1)};

  for (size_t k = 0; k < nrows; k++)
    fieldA[k * ncols] = BF_ALPHA * static_cast<double>(rank_offset + k) / edge;

  if (rank + 1 == csize) {
    double *bottom = fieldA + (nrows - 1) * ncols;
    for (size_t c = 0; c < ncols; c++)
      bottom[c] = BF_ALPHA * static_cast<double>(ncols - 1 - c) / edge;
  }
}


// ------------------------------------------------------------------------
// Misc.
// ------------------------------------------------------------------------
std::ostream & Mesh::print_rankAB(std::ostream &os) {
  #pragma acc update self(fieldA[0:msize],fieldB[0:msize])

  os << "Rank " << rank << " fields A and B:\n";

  for (size_t r = 0; r < nrows; r++) {
    for (size_t c = 0; c < ncols; c++) {
      os << std::left << std::setw(DEFAULT_WIDTH) << std::setprecision(DEFAULT_PRECISION) << fieldA[r * ncols + c];
    }
    os << "\n";
  }

  os << "\n";

  for (size_t r = 0; r < nrows; r++) {
    for (size_t c = 0; c < ncols; c++) {
      os << std::left << std::setw(DEFAULT_WIDTH) << std::setprecision(DEFAULT_PRECISION) << fieldB[r * ncols + c];
    }
    os << "\n";
  }

  return os << "\n\n";
}


std::ostream & Mesh::print(std::ostream &os) {
  #pragma acc host_data use_device(fieldA)
  {
    MPI_Gather(fieldA + 1 + ncols, 1, col_t, print_buff, N * (nrows - 2), MPI_DOUBLE, ROOT, comm);
  }
  
  if (rank) return os;

  for (size_t r = 0; r < N; r++) {
    for (size_t c = 0; c < N; c++) {
      os << std::setprecision(DEFAULT_PRECISION) << print_buff[r * N + c] << " ";
    }
    os << "\n";
  }
  return os;
}


void Mesh::exchange_B_halos() {
  nvtxRangePush("halo_exchange");

  const int N_int{static_cast<int>(N)};
  const size_t toprow_offset{1 + ncols}, bottomrow_offset{1 + (ncols * (nrows - 2))};

  #pragma acc host_data use_device(fieldB)
  {
    if (rank + 1 < csize) MPI_Isend(fieldB + bottomrow_offset, N_int, MPI_DOUBLE, rank + 1, rank, comm, &exchange_rqs[0]);
    if (rank > 0) MPI_Irecv(fieldB + 1, N_int, MPI_DOUBLE, rank - 1, rank - 1, comm, &exchange_rqs[1]);
    
    if (rank > 0) MPI_Isend(fieldB + 1 + ncols, N_int, MPI_DOUBLE, rank - 1, rank, comm, &exchange_rqs[2]);
    if (rank + 1 < csize) MPI_Irecv(fieldB + 1 + ncols * (nrows - 1), N_int, MPI_DOUBLE, rank + 1, rank + 1, comm, &exchange_rqs[3]);

    MPI_Waitall(4, exchange_rqs, MPI_STATUSES_IGNORE);
  }
  
  nvtxRangePop();
}


void Mesh::swapAB() {
  std::swap(fieldA, fieldB);
}


// ------------------------------------------------------------------------
// Solver
// ------------------------------------------------------------------------
void Mesh::iter() {
  nvtxRangePush("iter");
  
  double *A = fieldA;
  double *B = fieldB;
  size_t nr = nrows;
  size_t nc = ncols;

  #pragma acc parallel loop collapse(2) present(A[0:msize], B[0:msize])
  for (size_t r = 1; r < nr - 1; r++) {
    for (size_t c = 1; c < nc - 1; c++) {
      B[r * nc + c] = 0.25 * (A[r * nc + c - 1] +
                              A[r * nc + c + 1] +
                              A[(r - 1) * nc + c] +
                              A[(r + 1) * nc + c]);
    }
  }

  nvtxRangePop();
}


void Mesh::solve(size_t max_iters, size_t write_intvl, std::string_view filepath_prefix) {
  for (size_t i = 0; i < max_iters; i++) {
    if (write_intvl && i % write_intvl == 0) write_step(filepath_prefix, i, max_iters);
    iter();
    exchange_B_halos();
    swapAB();
  }
}


void Mesh::write_step(std::string_view filepath_prefix, size_t iter_idx, size_t max_iters) {
  std::string filepath = std::string(filepath_prefix) + std::to_string(iter_idx) + ".txt";
  std::ofstream out(filepath);
  print(out);
  out.close();
  return;
}