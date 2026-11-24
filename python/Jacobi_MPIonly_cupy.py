import os
import sys
import cupy as cp

# Define a custom RawKernel for the Jacobi computation
jacobi_kernel = cp.RawKernel(r'''
extern "C" __global__
void jacobi_kernel(const double* a, double* b, int height, int width) {
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    int i = blockIdx.y * blockDim.y + threadIdx.y;

    if (i > 0 && i < height - 1 && j > 0 && j < width - 1) {
        b[i * width + j] = 0.25 * (a[i * width + (j - 1)] +      // left
                                   a[i * width + (j + 1)] +      // right
                                   a[(i - 1) * width + j] +      // top
                                   a[(i + 1) * width + j]);     // bottom
    }
}
''', 'jacobi_kernel')

class Jacobi:
    def __init__(self, mesh, timer):
        self.mesh = mesh
        self.timer = timer

        # Move data to the current GPU device
        self.A = cp.asarray(self.mesh.fieldA)
        self.B = cp.asarray(self.mesh.fieldB)

    def iter_step(self):
        height, width = self.A.shape
        threadsperblock = (32, 32)
        blockspergrid_x = (width + threadsperblock[0] - 1) // threadsperblock[0]
        blockspergrid_y = (height + threadsperblock[1] - 1) // threadsperblock[1]
        blockspergrid = (blockspergrid_x, blockspergrid_y)

        # The kernel handles boundary conditions, so we can call it once.
        # However, to keep the timing consistent with the other versions,
        # we will maintain the compute/comm/compute structure.

        self.timer.start("compute1")
        jacobi_kernel(blockspergrid, threadsperblock, (self.A, self.B, height, width))
        cp.cuda.stream.get_current_stream().synchronize()
        self.timer.stop("compute1")

        self.timer.start("comm1")
        self.mesh.exchange_halos(self.B)
        self.timer.stop("comm1")

        self.timer.start("compute2")
        # In this model, the second compute block isn't strictly necessary
        # as the kernel computes the whole domain. We call it again to
        # ensure the logic matches the previous versions where halos are updated.
        jacobi_kernel(blockspergrid, threadsperblock, (self.A, self.B, height, width))
        cp.cuda.stream.get_current_stream().synchronize()
        self.timer.stop("compute2")
        
        self.timer.start("comm2")
        self.mesh.wait_for_halo_exchange()
        self.timer.stop("comm2")
        
        self.A, self.B = self.B, self.A

    def write_iter(self, it, path_prefix="./data/iter_"):
        out_dir = os.path.dirname(path_prefix.rstrip("/"))
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
            
        # Copy data from GPU to CPU for plotting
        self.mesh.fieldA = cp.asnumpy(self.A)
        save_path = f"{path_prefix}{it:04d}.png"
        self.mesh.plot(root=0, save_path=save_path)

    def solver(self, niters, write_freq=0, write_path_prefix="./data/iter_"):
        for it in range(niters):
            self.iter_step()

            if (write_freq) and (it % write_freq) == 0:
                self.write_iter(it, write_path_prefix)

        # Copy final result back to host
        self.mesh.fieldA = cp.asnumpy(self.A)
        self.mesh.fieldB = cp.asnumpy(self.B)