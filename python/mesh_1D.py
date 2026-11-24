from mpi4py import MPI

import numpy as np
import matplotlib.pyplot as plt


FIELD_INIT_VAL = 0.5


class Mesh:
    def __init__(self, field_size, mpicomm=MPI.COMM_WORLD):
        self.N = field_size
        self.comm = mpicomm

        self.rank = self.comm.Get_rank()
        self.csize = self.comm.Get_size()

        remaining_rows = self.N % self.csize
        self.all_nrows = np.array([self.N // self.csize + (1 if r < remaining_rows else 0) for r in range(self.csize)], dtype=np.uint64)
        self.nrows = self.all_nrows[self.rank]

        # Allocate one field with halos
        self.fieldA = np.full((self.nrows + 2, self.N + 2), FIELD_INIT_VAL, dtype=np.float64)

        # Zero out halos and padded region
        self.fieldA[:,  0] = 0.0
        self.fieldA[:, -1] = 0.0

        if self.rank == 0:
            self.fieldA[0, 1:-1] = 0.0
        if self.rank == self.csize - 1:
            self.fieldA[-1, 1:-1] = 0.0

        # Boundary condition
        # local row 0 (the top halo) is padded global row sum(rows of the ranks above)
        global_initial_row_idx = self.all_nrows[:self.rank].sum()
        gradient = 100 * (np.arange(global_initial_row_idx, global_initial_row_idx + self.nrows + 2, dtype=np.float64) / (self.N + 1))
        self.fieldA[:, 0] = gradient

        if self.rank == self.csize - 1:
            gradient = 100 * (np.arange(self.N + 1, -1, -1, dtype=np.float64) / (self.N + 1))
            self.fieldA[-1, :] = gradient

        self.fieldB = self.fieldA.copy()


    def exchange_halos(self, field=None):
        if field is None:
            field = self.fieldB

        self.reqs = []

        up = self.rank - 1 if self.rank > 0 else MPI.PROC_NULL
        down = self.rank + 1 if self.rank < self.csize - 1 else MPI.PROC_NULL

        self.reqs.append(self.comm.Irecv(field[-1, 1:-1], source=down, tag=101))
        self.reqs.append(self.comm.Irecv(field[ 0, 1:-1], source=up,   tag=102))

        self.reqs.append(self.comm.Isend(field[ 1, 1:-1], dest=up,   tag=101))
        self.reqs.append(self.comm.Isend(field[-2, 1:-1], dest=down, tag=102))


    def wait_for_halo_exchange(self, field=None):
        MPI.Request.Waitall(self.reqs)


    def plot_loc_fieldA(self):
        plt.imshow(self.fieldA,
                #    vmin=0, vmax=100,
                   cmap='hot', interpolation='nearest')
        plt.show()

    def plot_loc_fieldB(self):
        plt.imshow(self.fieldB,
                #    vmin=0, vmax=100,
                   cmap='hot', interpolation='nearest')
        plt.show()


    def plot(self, root=0, save_path=None):
        tile = np.ascontiguousarray(self.fieldA[1:-1, 1:-1], dtype=np.float64)
        sendbuf = tile.ravel()

        counts = [int(nr) * int(self.N) for nr in self.all_nrows]
        displs = [0]
        for i in range(1, self.csize):
            displs.append(displs[i - 1] + counts[i - 1])

        recvbuf = np.empty(self.N * self.N, dtype=np.float64) if self.rank == 0 else None
        self.comm.Gatherv(sendbuf, [recvbuf, counts, displs, MPI.DOUBLE], root=root)

        if self.rank != root:
            return

        field_glob = recvbuf.reshape((self.N, self.N))

        if save_path:
            plt.imsave(save_path, field_glob, vmin=0, vmax=100, cmap="hot")