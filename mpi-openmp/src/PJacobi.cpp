#include <cstring>
#include <cmath>

#include "Mesh.hpp"
#include "BoundaryFunction.hpp"
#include "PJacobi.hpp"

using field_t = double;
using TimeUnits = std::chrono::milliseconds;


// Argument parsing stuff
struct Options {
  size_t N{10};
  size_t niters{100};
};

Options parseArgs(int argc, char** argv) {
  Options opts{};
  for (int i = 1; i < argc; i++) {
    if ((std::strcmp(argv[i], "-N") == 0) && (i + 1 < argc)) {
      opts.N = std::stoull(argv[++i]);

    } else if ((std::strcmp(argv[i], "-I") == 0) && (i + 1 < argc)) {
      opts.niters = std::stoull(argv[++i]);

    } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      std::cout << "Usage: " << argv[0] << " [options]\n"
                << "  -N <int> field length in one axis.\n"
                << "  -I <int> total iters.\n";
        MPI_Finalize();
        std::exit(EXIT_SUCCESS);
    }
  }
  
  return opts;
}


int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int rank, size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  Options opts = parseArgs(argc, argv);
  
  if (toINT(opts.N) < size) {
    if (rank == 0) std::cerr << "mesh-dim must be >= nproc!\n";
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  if (opts.niters == 0) {
    if (rank == 0) std::cerr << "number of iterations must be > 0!\n";
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // Timer
  MPITiming::Timekeeper tk;

  MPITiming::Stopwatch sw(tk);
  sw.start("total");

  // The mesh
  auto bf = BF<field_t>;
  Mesh<field_t> mesh(opts.N, bf, tk);

  // The Solver
  PJacobi<field_t> solver(mesh, tk);
  solver.solve(opts.niters);

  sw.stop();

  tk.packet_header<TimeUnits>();
  if (rank == 0) std::cout << "\n";
  tk.print_all_packets<TimeUnits>();
  
  MPI_Finalize();
  return 0;
}