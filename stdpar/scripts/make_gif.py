"""Turn the field dumps from PJ.x / SJ.x into an animated gif (matplotlib version of plot_gif.gp).

    mkdir Mesh_Iters_Parallel
    build/TBB_REL/bin/PJ.x -l 1 -n 100 -i 12000 --print2file 1 -fpi 10
    python scripts/make_gif.py Mesh_Iters_Parallel results/jacobi.gif

Most of the change happens in the first few hundred sweeps, so the frames are spaced
roughly geometrically in the sweep number rather than evenly.
"""
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter

src = Path(sys.argv[1] if len(sys.argv) > 1 else "Mesh_Iters_Parallel")
out = Path(sys.argv[2] if len(sys.argv) > 2 else "jacobi.gif")
n_frames = 90

dumps = {int(f.stem): f for f in src.glob("*.dat")}
sweeps = np.array(sorted(dumps))
wanted = np.unique(np.r_[0, np.geomspace(max(sweeps[1], 1), sweeps[-1], n_frames)])
frames = sorted({int(sweeps[np.abs(sweeps - w).argmin()]) for w in wanted})
frames += [frames[-1]] * 15  # hold the steady state for a moment before looping

fig, ax = plt.subplots(figsize=(4.6, 4.6))
im = ax.imshow(np.loadtxt(dumps[frames[0]]), cmap="inferno", vmin=0, vmax=100, interpolation="nearest")
label = ax.text(0.03, 0.04, "", transform=ax.transAxes, color="white", fontsize=12, family="monospace")
ax.set_axis_off()
ax.set_title("Jacobi relaxing a heated plate", fontsize=12)
fig.tight_layout()


def draw(k):
    im.set_data(np.loadtxt(dumps[k]))
    label.set_text(f"sweep {k:>6,d}")
    return im, label


FuncAnimation(fig, draw, frames=frames, blit=True).save(out, writer=PillowWriter(fps=12), dpi=80)
print(f"wrote {out} ({len(frames)} frames, sweeps 0 to {frames[-1]:,})")
