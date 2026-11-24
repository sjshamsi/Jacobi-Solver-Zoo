import os
import sys

class Jacobi:
    def __init__(self, mesh, timer):
        self.mesh = mesh
        self.timer = timer

    def iter_step(self):
        A = self.mesh.fieldA
        B = self.mesh.fieldB

        self.timer.start("compute1")
        B[1, 1:-1] = 0.25 * (
            A[1, 0:-2] +
            A[1, 2:  ] +
            A[0, 1:-1] +
            A[2, 1:-1]
            )
        
        B[-2, 1:-1] = 0.25 * (
            A[-2, 0:-2] +
            A[-2, 2:  ] +
            A[-3, 1:-1] +
            A[-1, 1:-1]
            )
        self.timer.stop("compute1")

        self.timer.start("comm1")
        self.mesh.exchange_halos()
        self.timer.stop("comm1")

        self.timer.start("compute2")
        B[2:-2, 1:-1] = 0.25 * (
            A[2:-2, 0:-2] +
            A[2:-2, 2:  ] +
            A[1:-3, 1:-1] +
            A[3:-1, 1:-1]
            )
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