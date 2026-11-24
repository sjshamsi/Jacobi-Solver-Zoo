#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>
#include <vector>
#include <chrono>

#include "Print2DMatrix.hpp"
#include "CMesh.hpp"
#include "BoundaryFunction.hpp"
#include "ParallelJacobi.hpp"
#include "SerialJacobi.hpp"

using ::testing::Pointwise;
using ::testing::DoubleNear;


constexpr size_t submatrix_size{10};


struct Options {
  size_t n_solveloops{5};
  size_t field_dim{100};
  size_t max_iters{5000};
  bool print2term{false};
  size_t termprint_intrvl{100};
  std::ostream &os{std::cout};
  bool print2file{false};
  size_t fileprint_intrvl{10};
  std::string filedir{"./Mesh_Iters_Parallel/"};
};


TEST(JacobiDiff, ParallelMatchesSerial_PointwiseAbsTol) {
  Options opt;
  auto boundary_function = BF<double>;

  CMesh<double> p_mesh(opt.field_dim, boundary_function);
  CMesh<double> s_mesh(opt.field_dim, boundary_function);
  
  PJacobiSolver<double> p_solver(p_mesh);
  SJacobiSolver<double> s_solver(s_mesh);

  auto p_start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < opt.n_solveloops; i++) {
    p_solver.solve(opt.max_iters,
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
  auto p_end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> p_elapsed = p_end - p_start;


  auto s_start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < opt.n_solveloops; i++) {
    s_solver.solve(opt.max_iters,
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
  auto s_end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> s_elapsed = s_end - s_start;

  double p_elapsed_ms = p_elapsed.count();
  double s_elapsed_ms = s_elapsed.count();

  double p_avg = p_elapsed_ms / static_cast<double>(opt.n_solveloops);
  double s_avg = s_elapsed_ms / static_cast<double>(opt.n_solveloops);

  std::cout << "Serial vs. -03 par policy Parallel Jacobian\n"
            << opt.field_dim << "x" << opt.field_dim << " matrices, "
            << opt.max_iters << " iters, " << opt.n_solveloops << " loops.\n\n"

            << "Par.\t\tTotal " << p_elapsed_ms << "ms\t\tAvg. " << p_avg << "ms\n"
            << "Ser.\t\tTotal " << s_elapsed_ms << "ms\t\tAvg. " << s_avg << "ms\n";


  std::cout << "Top left corner of the matrices:\n\n";
  print2DMatrix<double>(p_mesh.fieldA, submatrix_size, submatrix_size);
  print2DMatrix<double>(s_mesh.fieldA, submatrix_size, submatrix_size);
  

  const std::vector<double> got = p_mesh.fieldA;
  const std::vector<double> exp = s_mesh.fieldA;


  ASSERT_EQ(got.size(), exp.size());
  const double abs_tol = 1e-9;

  // Checks |got[i] - exp[i]| <= abs_tol for all i
  EXPECT_THAT(got, Pointwise(DoubleNear(abs_tol), exp));
}
