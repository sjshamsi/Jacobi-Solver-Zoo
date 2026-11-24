import os
import sys

class Jacobi:
    def __init__(self, mesh, timer):
        self.mesh = mesh
        self.timer = timer

    def iter_step(self):
        A = self.mesh.fieldA
        B = self.mesh.fieldB
        nvx, nvy = self.mesh.n_valid_x, self.mesh.n_valid_y

        if nvx == 0 or nvy == 0:
            return
        
        self.timer.start("compute1")
        B[1, 1:nvx+1] = 0.25 * (
            A[1, 0:nvx] +
            A[1, 2:nvx+2] +
            A[0, 1:nvx+1] +
            A[2, 1:nvx+1]
            )
        
        B[nvy, 1:nvx+1] = 0.25 * (
            A[nvy, 0:nvx] +
            A[nvy, 2:nvx+2] +
            A[nvy-1, 1:nvx+1] +
            A[nvy+1, 1:nvx+1]
            )
        
        B[1:nvy+1, 1] = 0.25 * (
            A[1:nvy+1, 0] +
            A[1:nvy+1, 2] +
            A[0:nvy, 1] +
            A[2:nvy+2, 1]
            )
        
        B[1:nvy+1, nvx] = 0.25 * (
            A[1:nvy+1, nvx-1] +
            A[1:nvy+1, nvx+1] +
            A[0:nvy, nvx] +
            A[2:nvy+2, nvx]
            )
        self.timer.stop("compute1")

        self.timer.start("comm1")
        self.mesh.exchange_halos()
        self.timer.stop("comm1")

        self.timer.start("compute2")
        if nvy >= 3 and nvx >= 3:
            B[2:nvy, 2:nvx] = 0.25 * (
                A[1:nvy-1, 2:nvx] +
                A[3:nvy+1, 2:nvx] +
                A[2:nvy,   1:nvx-1] +
                A[2:nvy,   3:nvx+1]
                )
        self.timer.stop("compute2")
            
        self.timer.start("comm2")
        self.mesh.wait_for_halo_exchange()
        self.timer.stop("comm2")

        # Alternatively, the Line below updates everything in one go
        # B[1:nvy+1, 1:nvx+1] = 0.25 * (A[0:nvy, 1:nvx+1] + A[2:nvy+2, 1:nvx+1] + A[1:nvy+1, 0:nvx] + A[1:nvy+1, 2:nvx+2])
        # self.mesh.exchange_halos()
        # self.mesh.wait_for_halo_exchange()

        self.mesh.fieldA, self.mesh.fieldB = self.mesh.fieldB, self.mesh.fieldA

    def write_iter(self, it, path_prefix="./data/iter_"):
        out_dir = os.path.dirname(path_prefix.rstrip("/"))
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
            
            
        save_path = f"{path_prefix}{it:04d}.png"
        self.mesh.plot(cart_root=0, save_path=save_path)

    def solver(self, niters, write_freq=0, write_path_prefix="./data/iter_"):
        for it in range(niters):
            self.iter_step()

            if (write_freq) and (it % write_freq) == 0:
                self.write_iter(it, write_path_prefix)