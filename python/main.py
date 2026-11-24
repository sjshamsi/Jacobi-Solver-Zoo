import argparse
import importlib

from mpi_timer import MPITimer

# Which solver module to use. Every variant has the same Jacobi class interface;
# only slicing_2D needs the 2D (Cartesian) mesh, the rest split the rows over ranks.
VARIANTS = {
    "noslice":    ("Jacobi_MPIonly_noslice",    "mesh_1D"),
    "slicing_1D": ("Jacobi_MPIonly_slicing_1D", "mesh_1D"),
    "slicing_2D": ("Jacobi_MPIonly_slicing_2D", "mesh_2D"),
    "jit":        ("Jacobi_MPIonly_jit",        "mesh_1D"),
    "njit":       ("Jacobi_MPIonly_njit",       "mesh_1D"),
    "jit_cuda":   ("Jacobi_MPIonly_jit_cuda",   "mesh_1D"),
    "cupy":       ("Jacobi_MPIonly_cupy",       "mesh_1D"),
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MPI Jacobi solver with a choice of Python back ends.")
    parser.add_argument("--variant", choices=VARIANTS, default="cupy")
    parser.add_argument("--N", type=int, default=50_000, help="field size (N x N)")
    parser.add_argument("--niters", type=int, default=1000)
    parser.add_argument("--write_freq", type=int, default=0, help="save a PNG every this many iterations (0 = never)")
    args = parser.parse_args()

    solver_mod, mesh_mod = VARIANTS[args.variant]
    Jacobi = importlib.import_module(solver_mod).Jacobi
    Mesh = importlib.import_module(mesh_mod).Mesh

    timer = MPITimer()
    mesh = Mesh(args.N)
    solver = Jacobi(mesh, timer)

    solver.solver(args.niters, write_freq=args.write_freq)

    mesh.comm.Barrier()
    timer.report(mesh.comm)
