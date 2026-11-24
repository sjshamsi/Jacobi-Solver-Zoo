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

        # Making the cartesian grid
        dims = MPI.Compute_dims(self.csize, 2)
        self.cart = self.comm.Create_cart(dims=dims, periods=(False, False), reorder=True)
        coords = self.cart.Get_coords(self.cart.rank)
        self.Py,        self.Px     = dims
        self.Py_loc,    self.Px_loc = coords

        # Local sizes
        self.nloc_x = (self.N + self.Px - 1) // self.Px
        self.nloc_y = (self.N + self.Py - 1) // self.Py

        # Global padded size
        self.Npad_x = self.nloc_x * self.Px
        self.Npad_y = self.nloc_y * self.Py

        # Global start indices of this rank’s physical block (no halo)
        self.i_0 = self.Px_loc * self.nloc_x
        self.j_0 = self.Py_loc * self.nloc_y

        # the extents of this rank
        self.n_valid_x = max(0, min(self.nloc_x, self.N - self.i_0))
        self.n_valid_y = max(0, min(self.nloc_y, self.N - self.j_0))

        # Setting neighbours
        self.nbr_up, self.nbr_down = self.cart.Shift(0, 1)
        self.nbr_left, self.nbr_right = self.cart.Shift(1, 1)

        # Allocate one field with halos
        self.fieldA = np.full((self.nloc_y + 2, self.nloc_x + 2), FIELD_INIT_VAL, dtype=np.float64)

        # Zero out halos and padded region
        if self.Py_loc == 0:
            self.fieldA[0, :] = 0.0
        if self.Py_loc == self.Py - 1:
            self.fieldA[self.n_valid_y + 1:, :] = 0.0

        if self.Px_loc == 0:
            self.fieldA[:, 0] = 0.0
        if self.Px_loc == self.Px - 1:
            self.fieldA[:, self.n_valid_x + 1:] = 0.0

        # Boundary condition
        if self.Py_loc == (self.Py - 1):
            _is = np.arange(self.i_0, self.i_0 + self.n_valid_x + 2, dtype=np.float64)
            gradient = 100.0 * (((self.N + 1) - _is) / (self.N + 1))
            self.fieldA[self.n_valid_y + 1, :self.n_valid_x + 2] = gradient

        if self.Px_loc == 0:
            _js = np.arange(self.j_0, self.j_0 + self.n_valid_y + 2, dtype=np.float64)
            gradient = 100.0 * (_js / (self.N + 1))
            self.fieldA[:self.n_valid_y + 2, 0] = gradient

        self.fieldB = self.fieldA.copy()



    def exchange_halos(self, field=None):
        if field is None:
            field = self.fieldB

        if self.n_valid_x == 0 or self.n_valid_y == 0:
            return
        
        self.reqs = []

        recv_top = field[0, 1:1+self.n_valid_x]
        send_top = field[1, 1:1+self.n_valid_x]
        self.reqs.append(self.cart.Irecv(recv_top, source=self.nbr_up,   tag=101))
        self.reqs.append(self.cart.Isend(send_top, dest=self.nbr_up,     tag=102))

        recv_bot = field[self.n_valid_y+1, 1:1+self.n_valid_x]
        send_bot = field[self.n_valid_y,   1:1+self.n_valid_x]
        self.reqs.append(self.cart.Irecv(recv_bot, source=self.nbr_down, tag=102))
        self.reqs.append(self.cart.Isend(send_bot, dest=self.nbr_down,   tag=101))

        self.recv_left_buf = np.empty(self.n_valid_y, dtype=np.float64)
        send_left_buf = np.ascontiguousarray(field[1:1+self.n_valid_y, 1])
        self.reqs.append(self.cart.Irecv(self.recv_left_buf, source=self.nbr_left,  tag=201))
        self.reqs.append(self.cart.Isend(send_left_buf, dest=self.nbr_left,    tag=202))

        self.recv_right_buf = np.empty(self.n_valid_y, dtype=np.float64)
        send_right_buf = np.ascontiguousarray(field[1:1+self.n_valid_y, self.n_valid_x])
        self.reqs.append(self.cart.Irecv(self.recv_right_buf, source=self.nbr_right, tag=202))
        self.reqs.append(self.cart.Isend(send_right_buf, dest=self.nbr_right,   tag=201))


    def wait_for_halo_exchange(self, field=None):
        if field is None:
            field = self.fieldB

        MPI.Request.Waitall(self.reqs)
        if self.nbr_left != MPI.PROC_NULL:
            field[1:1+self.n_valid_y, 0] = self.recv_left_buf
        if self.nbr_right != MPI.PROC_NULL:
            field[1:1+self.n_valid_y, self.n_valid_x+1] = self.recv_right_buf


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


    def plot(self, cart_root=0, save_path=None):
        my_cart_rank = self.cart.Get_rank()
        tile_slice = (slice(1, self.n_valid_y + 1), slice(1, self.n_valid_x + 1))

        if my_cart_rank != cart_root:
            self.cart.Send([np.ascontiguousarray(self.fieldA[tile_slice]), MPI.DOUBLE], dest=cart_root, tag=0)
            return
        
        field_glob = np.empty((self.N, self.N), dtype=np.float64)

        # Place root's own tile
        field_glob[self.j_0:self.j_0 + self.n_valid_y, self.i_0:self.i_0 + self.n_valid_x] = self.fieldA[tile_slice]

        reqs = []
        types = []

        for r in range(self.cart.Get_size()):
            if r == cart_root:
                continue

            r_Py_loc, r_Px_loc = self.cart.Get_coords(r)

            r_i_0 = r_Px_loc * self.nloc_x
            r_j_0 = r_Py_loc * self.nloc_y

            r_n_valid_x = max(0, min(self.nloc_x, self.N - r_i_0))
            r_n_valid_y = max(0, min(self.nloc_y, self.N - r_j_0))

            # If a rank owns no physical cells, skip it
            if r_n_valid_x == 0 or r_n_valid_y == 0:
                continue

            sub = MPI.DOUBLE.Create_subarray(
                sizes=(self.N, self.N),
                subsizes=(r_n_valid_y, r_n_valid_x),
                starts=(r_j_0, r_i_0),
                order=MPI.ORDER_C
            )
            sub.Commit()
            types.append(sub)

            reqs.append(self.cart.Irecv([field_glob, 1, sub], source=r, tag=0))

        MPI.Request.Waitall(reqs)
        for sub in types:
            sub.Free()
        
        if save_path:
            plt.imsave(save_path, field_glob, vmin=0, vmax=100, cmap="hot")