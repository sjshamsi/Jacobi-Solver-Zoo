#include <fstream>
#include <chrono>

#include "Print2DMatrix.hpp"
#include "CMesh.hpp"
#include "BoundaryFunction.hpp"
#include "SerialJacobi.hpp"

#include <cstring>

using field_t = double;

struct Options {
  size_t n_solveloops{3};
  size_t field_dim{10};
  size_t max_iters{1000};
  bool print2term{false};
  size_t termprint_intrvl{100};
  std::ostream &os{std::cout};
  bool print2file{false};
  size_t fileprint_intrvl{10};
  std::string filedir{"./Mesh_Iters_Serial/"};
};


Options parse_args(int argc, char** argv) {
  Options opt;

  for (int i = 1; i < argc; i++) {
    if ( (std::strcmp(argv[i], "--n-solveloops") == 0 || std::strcmp(argv[i], "-l") == 0) && i + 1 < argc ) {
      opt.n_solveloops = std::stoull(argv[i + 1]);

    } else if ( (std::strcmp(argv[i], "--field-dim") == 0 || std::strcmp(argv[i], "-n") == 0) && i + 1 < argc ) {
      opt.field_dim = std::stoull(argv[i + 1]);

    } else if ( (std::strcmp(argv[i], "--max-iters") == 0 || std::strcmp(argv[i], "-i") == 0) && i + 1 < argc ) {
      opt.max_iters = std::stoull(argv[i + 1]);

    } else if ( (std::strcmp(argv[i], "--print2term") == 0) && (i + 1 < argc) ) {
      opt.print2term = std::stoull(argv[i + 1]) == 1 ? true : false;

    } else if ( (std::strcmp(argv[i], "--termprint-intrvl") == 0 || std::strcmp(argv[i], "-tpi") == 0) && i + 1 < argc ) {
      opt.termprint_intrvl = std::stoull(argv[i + 1]);

    } else if ( (std::strcmp(argv[i], "--print2file") == 0) && i + 1 < argc ){
      opt.print2file = std::stoull(argv[i + 1]) == 1 ? true : false;

    } else if ( (std::strcmp(argv[i], "--fileprint_intrvl") == 0 || std::strcmp(argv[i], "-fpi") == 0) && i + 1 < argc ) {
      opt.fileprint_intrvl = std::stoull(argv[i + 1]);

    } else if ( (std::strcmp(argv[i], "--filedir") == 0 || std::strcmp(argv[i], "-fn") == 0) && i + 1 < argc ) {
      opt.filedir = argv[i + 1];

    } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      std::cout << "msage: " << argv[0] << " [options]\n"
                << "  -n, --field-dim <N>   Number of elements in the vector (default 7)\n";
      std::exit(0);
    }
  }

  return opt;
}

int main(int argc, char** argv) {
  Options opt = parse_args(argc, argv);

  if (opt.field_dim == 0)  {
    std::cerr << "field-dim mmst be > 0\n";
    exit(EXIT_FAILURE);
  }
  if (opt.max_iters == 0)  {
    std::cerr << "max-iters mmst be > 0\n";
    exit(EXIT_FAILURE);
  }

  auto boundary_function = BF<field_t>;
  CMesh<field_t> mesh(opt.field_dim, boundary_function);
  SJacobiSolver<field_t> solver(mesh);

  std::ofstream outfile("log.txt", std::ios::app);
  if (! outfile) {
    std::cerr << "Error: could not open file\n";
    return 1;
  }

  std::string job = "P. Jacobi " +
                    std::to_string(opt.field_dim) + "x" + std::to_string(opt.field_dim) + ", " +
                    std::to_string(opt.max_iters) + " iters";

  auto start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < opt.n_solveloops; i++) {
    solver.solve(opt.max_iters,
                 DEFAULT_WIDTH,
                 DEFAULT_PRECISION,
                 opt.print2term,
                 opt.termprint_intrvl,
                 std::cout,
                 opt.print2file,
                 opt.fileprint_intrvl,
                 opt.filedir
                );
  }
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end - start;

  double elapsed_ms = elapsed.count();
  double avg = elapsed_ms / static_cast<double>(opt.n_solveloops);

  outfile << job << ": "
  << elapsed_ms << ' ' << "ms"
  << " total in " << opt.n_solveloops << " calls"
  << " (avg " << avg << ' ' << "ms" << ")\n";

  outfile << elapsed_ms << ", " << opt.n_solveloops << ", " << avg << ", "
          << opt.field_dim << ", " << opt.max_iters << "\n";
  
  outfile.close();

  std::cout << "This is junk. The (1, 1) element of the vector is " << mesh.fieldA[mesh.N + 3] << "\n";

  return 0;
}