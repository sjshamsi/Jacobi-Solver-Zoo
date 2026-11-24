import os
import sys
from numba import njit, prange

@njit(parallel=True)
def jacobi_compute_top_bottom(A, B):
    # Top boundary
    for j in prange(1, A.shape[1] - 1):
        B[1, j] = 0.25 * (A[1, j - 1] + A[1, j + 1] + A[0, j] + A[2, j])
    # Bottom boundary
    for j in prange(1, A.shape[1] - 1):
        B[-2, j] = 0.25 * (A[-2, j - 1] + A[-2, j + 1] + A[-3, j] + A[-1, j])


@njit(parallel=True)
def jacobi_compute_inner(A, B):
    # Inner domain
    for i in prange(2, A.shape[0] - 2):
        for j in range(1, A.shape[1] - 1):
            B[i, j] = 0.25 * (A[i, j - 1] + A[i, j + 1] + A[i - 1, j] + A[i + 1, j])


class Jacobi:
    def __init__(self, mesh, timer):
        self.mesh = mesh
        self.timer = timer

    def iter_step(self):
        A = self.mesh.fieldA
        B = self.mesh.fieldB

        self.timer.start("compute1")
        jacobi_compute_top_bottom(A, B)
        self.timer.stop("compute1")

        self.timer.start("comm1")
        self.mesh.exchange_halos()
        self.timer.stop("comm1")

        self.timer.start("compute2")
        jacobi_compute_inner(A, B)
        self.timer.stop("compute2")
        
        self.timer.start("comm2")
        self.mesh.wait_for_halo_exchange()
        self.timer.stop("comm2")

        self.mesh.fieldA, self.mesh.fieldB = self.mesh.fieldB, self.mesh.fieldA

    def write_iter(self, it, path_prefix="./data/iter_"):
        out_dir = os.path.dirname(path_prefix.rstrip("/"))
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
            
            
        save_path = f"{path_prefix}{it:04d}.png"
        self.mesh.plot(root=0, save_path=save_path)

    def solver(self, niters, write_freq=0, write_path_prefix="./data/iter_"):
        for it in range(niters):
            self.iter_step()

            if (write_freq) and (it % write_freq) == 0:
                self.write_iter(it, write_path_prefix)