#include <cstring>

#include "Mesh.hpp"

struct Options {
  size_t field_dim{57'600};
  size_t max_iters{20};
  size_t write_intvl{0};
};

Options parseArgs(int argc, char** argv) {
  Options opts{};
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "-N") == 0 && i + 1 < argc) {
      opts.field_dim = std::stoull(argv[++i]);

    } else if (std::strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
      opts.max_iters = std::stoull(argv[++i]);

    } else if (std::strcmp(argv[i], "-WI") == 0 && i + 1 < argc) {
      opts.write_intvl = std::stoull(argv[++i]);

    } else if (std::strcmp(argv[i], "-h") == 0) {
      std::cout
        << "Usage: " << argv[0] << " [options]\n"
        << "  -N <int>  Field dimension in one axis (default 57600)\n"
        << "  -I <int>  Max iterations (default 20)\n"
        << "  -WI <int> Write interval (default 0 = never)\n";
        std::exit(0);
    }
  }
  
  return opts;
}

int main(int argc, char **argv) {
  Options opts = parseArgs(argc, argv);
  if (opts.field_dim < 1) {
    std::cerr << "Field size must be at least 1\n";
    std::exit(EXIT_FAILURE);
  }

  if (opts.max_iters < 0) {
    std::cerr << "Max iteration number must be greater than 0\n";
    std::exit(EXIT_FAILURE);
  }

  MPI_Init(&argc, &argv);
  nvtxRangePush("total");

  Mesh mesh(opts.field_dim);
  mesh.solve(opts.max_iters, opts.write_intvl, "./data/iter_");

  MPI_Finalize();
  nvtxRangePop();
  return 0;
}