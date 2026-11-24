from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyArrowPatch

# Conceptual banner: the temperature field every solver in this repo computes, split into
# row blocks (one per MPI rank) with the halo rows they swap, next to the 5-point stencil.
here = Path(__file__).parent
N, ranks = 120, 4

# Same boundary conditions as the solvers: left edge rises 0 -> 100 going down,
# bottom edge falls 100 -> 0 going right, top and right edges held at 0.
F = np.full((N + 2, N + 2), 0.5)
F[0, :] = F[:, -1] = 0.0
F[:, 0] = 100.0 * np.arange(N + 2) / (N + 1)
F[-1, :] = 100.0 * np.arange(N + 1, -1, -1) / (N + 1)
G = F.copy()
for _ in range(40000):  # roughly 3 N^2 sweeps, close to converged
    G[1:-1, 1:-1] = 0.25 * (F[:-2, 1:-1] + F[2:, 1:-1] + F[1:-1, :-2] + F[1:-1, 2:])
    F, G = G, F

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5.2), gridspec_kw={"width_ratios": [1.25, 1]})

ax1.imshow(F, cmap="inferno", vmin=0, vmax=100, extent=(0, N + 2, N + 2, 0))
rows = N // ranks
for r in range(ranks):
    top = 1 + r * rows
    ax1.add_patch(Rectangle((0.3, top), N + 1.4, rows, fill=False, ec="white", lw=1.4, ls="-"))
    ax1.text(N + 3.5, top + rows / 2, f"rank {r}", va="center", fontsize=11)
    if r:
        for y in (top - 1.5, top + 0.5):
            ax1.plot([1, N + 1], [y, y], color="#7fd3ff", lw=1.2, ls="--")
        ax1.add_patch(FancyArrowPatch((N * 0.18, top - 4), (N * 0.18, top + 4), arrowstyle="<|-|>",
                                      mutation_scale=12, color="#7fd3ff", lw=1.4))
ax1.text(N * 0.22, 1 + rows - 4.5, "halo rows swapped\nevery sweep", color="#7fd3ff", fontsize=9.5, va="bottom")
ax1.set_xlim(0, N + 16)
ax1.axis("off")
ax1.set_title("Steady-state temperature, rows split over MPI ranks", fontsize=11.5)

# Right: the stencil on a little grid
ax2.set_xlim(-0.2, 5.0); ax2.set_ylim(-0.9, 5.0); ax2.set_aspect("equal"); ax2.axis("off")
for i in range(5):
    for j in range(5):
        ax2.add_patch(Rectangle((j, 4 - i - 0.0), 0.92, 0.92, color="#eeeeee"))
cells = {(2, 2): ("#d62728", r"$B_{i,j}$"), (1, 2): ("#3b6ea5", r"$A_{i-1,j}$"), (3, 2): ("#3b6ea5", r"$A_{i+1,j}$"),
         (2, 1): ("#3b6ea5", r"$A_{i,j-1}$"), (2, 3): ("#3b6ea5", r"$A_{i,j+1}$")}
for (i, j), (col, lab) in cells.items():
    ax2.add_patch(Rectangle((j, 4 - i), 0.92, 0.92, color=col))
    ax2.text(j + 0.46, 4 - i + 0.46, lab, ha="center", va="center", color="white", fontsize=10.5)
ax2.text(2.3, -0.45, r"$B_{i,j} = \frac{1}{4}\,(A_{i-1,j} + A_{i+1,j} + A_{i,j-1} + A_{i,j+1})$, then swap A and B",
         ha="center", fontsize=11)
ax2.set_title("The 5-point Jacobi stencil", fontsize=11.5)

fig.suptitle("One Laplace solver, written in C++ stdpar, MPI + OpenMP, MPI + OpenACC and Python",
             fontsize=12.5)
fig.tight_layout()
fig.savefig(here / "banner.png", dpi=150, bbox_inches="tight")
print("wrote", here / "banner.png")
