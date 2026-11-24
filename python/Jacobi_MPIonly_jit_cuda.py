import os
import sys
from numba import cuda
import numpy as np

@cuda.jit
def jacobi_compute_top_bottom_cuda(A, B):
    j = cuda.grid(1)

    if 1 <= j < A.shape[1] - 1:
        # Top boundary
        B[1, j] = 0.25 * (A[1, j - 1] + A[1, j + 1] + A[0, j] + A[2, j])
        # Bottom boundary
        B[-2, j] = 0.25 * (A[-2, j - 1] + A[-2, j + 1] + A[-3, j] + A[-1, j])

@cuda.jit
def jacobi_compute_inner_cuda(A, B):
    i, j = cuda.grid(2)

    if 2 <= i < A.shape[0] - 2 and 1 <= j < A.shape[1] - 1:
        B[i, j] = 0.25 * (A[i, j - 1] + A[i, j + 1] + A[i - 1, j] + A[i + 1, j])


class Jacobi:
    def __init__(self, mesh, timer):
        self.mesh = mesh
        self.timer = timer

        ngpus = len(cuda.gpus)
        device_id = self.mesh.rank % ngpus
        cuda.select_device(device_id)

        self.A = cuda.to_device(self.mesh.fieldA)
        self.B = cuda.to_device(self.mesh.fieldB)

    def iter_step(self):
        threadsperblock_1d = 256
        blockspergrid_1d = (self.A.shape[1] + threadsperblock_1d - 1) // threadsperblock_1d
        
        threadsperblock_2d = (32, 32)
        blockspergrid_2d_x = (self.A.shape[0] + threadsperblock_2d[0] - 1) // threadsperblock_2d[0]
        blockspergrid_2d_y = (self.A.shape[1] + threadsperblock_2d[1] - 1) // threadsperblock_2d[1]
        blockspergrid_2d = (blockspergrid_2d_x, blockspergrid_2d_y)

        self.timer.start("compute1")
        jacobi_compute_top_bottom_cuda[blockspergrid_1d, threadsperblock_1d](self.A, self.B)
        cuda.synchronize()
        self.timer.stop("compute1")

        self.timer.start("comm1")
        self.mesh.exchange_halos(self.B)
        self.timer.stop("comm1")

        self.timer.start("compute2")
        jacobi_compute_inner_cuda[blockspergrid_2d, threadsperblock_2d](self.A, self.B)
        cuda.synchronize()
        self.timer.stop("compute2")
        
        self.timer.start("comm2")
        self.mesh.wait_for_halo_exchange()
        self.timer.stop("comm2")
        
        self.A, self.B = self.B, self.A

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
                self.mesh.fieldA = self.A.copy_to_host()
                self.mesh.fieldB = self.B.copy_to_host()
                
                self.write_iter(it, write_path_prefix)

                self.A = cuda.to_device(self.mesh.fieldA)
                self.B = cuda.to_device(self.mesh.fieldB)

        self.mesh.fieldA = self.A.copy_to_host()
        self.mesh.fieldB = self.B.copy_to_host()
