"""Check that every CPU variant gives the same field as a plain serial NumPy Jacobi.

    mpiexec -n 4 python compare_variants.py --N 50 --niters 300

Each variant runs on all ranks, the field is gathered on rank 0 (through mesh.plot, with
plt.imsave swapped for a function that just keeps the array), and rank 0 compares it with a
serial solve of the same problem. Try a rank count that does not divide N to test the
uneven row split.
"""
import argparse
import importlib

import numpy as np
from mpi4py import MPI

from main import VARIANTS
from mpi_timer import MPITimer

CPU_VARIANTS = ["noslice", "slicing_1D", "slicing_2D", "jit", "njit"]


def serial_reference(N, niters):
    F = np.full((N + 2, N + 2), 0.5)
    F[0, :] = F[:, -1] = 0.0
    F[:, 0] = 100.0 * np.arange(N + 2) / (N + 1)            # left: 0 at the top, rising downwards
    F[-1, :] = 100.0 * np.arange(N + 1, -1, -1) / (N + 1)   # bottom: falling to 0 at the right
    G = F.copy()
    for _ in range(niters):
        G[1:-1, 1:-1] = 0.25 * (F[:-2, 1:-1] + F[2:, 1:-1] + F[1:-1, :-2] + F[1:-1, 2:])
        F, G = G, F
    return F[1:-1, 1:-1]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--N", type=int, default=50)
    parser.add_argument("--niters", type=int, default=300)
    parser.add_argument("--tol", type=float, default=1e-10)
    args = parser.parse_args()
    rank = MPI.COMM_WORLD.Get_rank()

    ref = serial_reference(args.N, args.niters) if rank == 0 else None
    failed = False
    for name in CPU_VARIANTS:
        solver_mod, mesh_mod = VARIANTS[name]
        mesh_module = importlib.import_module(mesh_mod)
        got = {}
        mesh_module.plt.imsave = lambda path, arr, **kw: got.setdefault("field", arr.copy())

        mesh = mesh_module.Mesh(args.N)
        importlib.import_module(solver_mod).Jacobi(mesh, MPITimer()).solver(args.niters)
        mesh.plot(save_path="gather")

        if rank == 0:
            err = np.abs(got["field"] - ref).max()
            ok = err <= args.tol
            failed |= not ok
            print(f"{name:12s} {MPI.COMM_WORLD.Get_size()} ranks  max |diff| = {err:.2e}  {'ok' if ok else 'FAIL'}")
    if rank == 0 and failed:
        raise SystemExit(1)
