#pragma once

#include <algorithm>
#include <execution>
#include <vector>
#include <iomanip>
#include <ranges>

#include "constants.hpp"


template <typename T>
class PJacobiSolver {
  public:
    explicit PJacobiSolver(CMesh<T> &cmesh_instance);
    
    void solve(size_t max_iters,
               size_t print_width = DEFAULT_WIDTH,
               int print_precision = DEFAULT_PRECISION,
               bool print2term = false,
               size_t termprint_intvl = 1000,
               std::ostream &termos = std::cout,
               bool print2file = false,
               size_t fileprint_intvl = 500,
               std::string file_dir = "./Mesh_Iters_Parallel/"
              );

  private:
    CMesh<T> &mesh;

    void print_iter(std::ostream &os,
                    size_t print_width,
                    int print_precision
                  );
    
    void write_iter(std::string file_dir,
                    size_t iter_num,
                    size_t max_iter
                  );
};


template <typename T>
PJacobiSolver<T>::PJacobiSolver(CMesh<T> &cmesh_instance) : mesh(cmesh_instance) {}


template <typename T>
void PJacobiSolver<T>::print_iter(std::ostream &os,
                                 size_t print_width,
                                 int print_precision
                                ) { mesh.print(print_width, print_precision, os); }


template <typename T>
void PJacobiSolver<T>::write_iter(std::string file_dir,
                                 size_t iter_num,
                                 size_t max_iter
                                ) {
    std::ostringstream oss;
    int filename_width { numIntDigits(max_iter) };
    oss << file_dir << std::setfill('0') << std::setw(filename_width) << iter_num << ".dat";
    mesh.print_to_file(oss.str());
}


template <typename T>
void PJacobiSolver<T>::solve(size_t max_iters,
                            size_t print_width,
                            int print_precision,
                            bool print2term,
                            size_t termprint_intvl,
                            std::ostream &termos,
                            bool print2file,
                            size_t fileprint_intvl,
                            std::string file_dir
                          ) {
  
  // Lambda function that returns r/c given an array number
  auto rc = [=, *this](size_t i){ return std::pair{i / (mesh.M), i % (mesh.M)}; };
  
  // lambda function that tells if a cell is in the interior
  auto is_interior = [=, *this](size_t i) {
    auto [r, c] = rc(i);
    return (r != 0) && (r != mesh.N + 1) && (c != 0) && (c != mesh.N + 1);
  };
  
  auto all_idx = std::views::iota(static_cast<size_t>(0), mesh.fsize);

  std::vector<std::size_t> idx;
  idx.reserve(mesh.N * mesh.N);

  for (size_t i : all_idx)
    if (is_interior(i)) idx.push_back(i);

  // Begin the iterations!
  size_t count{0};

  while (count <= max_iters) {
    if ((print2term) && (count % termprint_intvl == 0)) print_iter(termos, print_width, print_precision);
    if ((print2file) && (count % fileprint_intvl == 0)) write_iter(file_dir, count, max_iters);

    std::for_each(std::execution::par,
      idx.begin(), idx.end(),
      [&](std::size_t i) {
        mesh.fieldB[i] = 0.25 * (mesh.fieldA[i - 1] + mesh.fieldA[i + 1] + mesh.fieldA[i + mesh.M] + mesh.fieldA[i - mesh.M]);
      });

    std::swap(mesh.fieldA, mesh.fieldB);
    count++;
  }
}