#pragma once

#include <iomanip>
#include <vector>

#include "constants.hpp"


template <typename T>
class SJacobiSolver {
  public:
    explicit SJacobiSolver(CMesh<T> &cmesh_instance);
    void solve(size_t max_iters,
               size_t print_width = DEFAULT_WIDTH,
               int print_precision = DEFAULT_PRECISION,
               bool print2term = false,
               size_t termprint_intvl = 1000,
               std::ostream &termos = std::cout,
               bool print2file = false,
               size_t fileprint_intvl = 500,
               std::string file_dir = "./Mesh_Iters_Serial/"
              );

  private:
    CMesh<T> &mesh;

    void print_iter(std::ostream &os, size_t print_width, int print_precision);
    void write_iter(std::string file_dir, size_t iter_num, size_t max_iter);

    void jacobi_iter();
};


template <typename T>
SJacobiSolver<T>::SJacobiSolver( CMesh<T> &mesh_instance) : mesh(mesh_instance) {}


template <typename T>
void SJacobiSolver<T>::solve(size_t max_iters,
                            size_t print_width,
                            int print_precision,
                            bool print2term,
                            size_t termprint_intvl,
                            std::ostream &termos,
                            bool print2file,
                            size_t fileprint_intvl,
                            std::string file_dir) {
  size_t count{0};
  
  while (count <= max_iters) {
    if ((print2term) && (count % termprint_intvl == 0)) print_iter(termos, print_width, print_precision);
    if ((print2file) && (count % fileprint_intvl == 0)) write_iter(file_dir, count, max_iters);
    
    jacobi_iter();
    std::swap(mesh.fieldA, mesh.fieldB);
    
    count++;
  }
}


template <typename T>
void SJacobiSolver<T>::jacobi_iter() {
  for (size_t i = 1; i <= mesh.N; i++) {
    for (size_t j = 1; j <= mesh.N; j++) {
      
      mesh.fieldB[i * (mesh.N + 2) + j] = 0.25 * ( mesh.fieldA[(i - 1) * (mesh.N + 2) + j] + mesh.fieldA[(i + 1) * (mesh.N + 2) + j] +
                                                   mesh.fieldA[i * (mesh.N + 2) + (j - 1)] + mesh.fieldA[i * (mesh.N + 2) + (j + 1)] );
    }
  }
}

template <typename T>
void SJacobiSolver<T>::print_iter(std::ostream &os,
                                 size_t print_width,
                                 int print_precision
                                ) { mesh.print(print_width, print_precision, os); }

template <typename T>
void SJacobiSolver<T>::write_iter(std::string file_dir, size_t iter_num, size_t max_iter) {
  std::ostringstream oss;
  int filename_width { numIntDigits(max_iter) };
  oss << file_dir << std::setfill('0') << std::setw(filename_width) << iter_num << ".dat";
  mesh.print_to_file(oss.str());
}