import csv
import re
from pathlib import Path
import matplotlib.pyplot as plt

# Thread sweeps of PJ.x (NV_REL preset: nvc++ -stdpar=multicore) on one Leonardo DCGP node.
# Each log line is one run: "# <T> CPUs, ... <N>x<N>, <I> iters ... (avg <ms> ms)".
# solve() runs I + 1 sweeps (the loop is `while (count <= max_iters)`), so time per sweep = avg / (I + 1).
here = Path(__file__).parent
pat = re.compile(r"#\s*(\d+) CPUs, (?:Parallel Jacobi|P\. Jacobi) (\d+)x\d+,? (\d+) iter\w*.*?\(avg ([\d.]+) ms\)")

runs = []
for log in sorted((here / "logs").glob("*.log")):
    for m in pat.finditer(log.read_text()):
        T, N, iters, avg = int(m[1]), int(m[2]), int(m[3]), float(m[4])
        runs.append(dict(log=log.name, threads=T, N=N, iters=iters, avg_ms=avg,
                         ms_per_sweep=round(avg / (iters + 1), 4)))
with (here / "threads.csv").open("w", newline="") as fh:
    w = csv.DictWriter(fh, fieldnames=list(runs[0])); w.writeheader(); w.writerows(runs)

series = [  # (label, filter, colour, marker, hollow)
    ("1000 × 1000 (8 MB per field)", lambda r: r["N"] == 1000, "#3b6ea5", "o", False),
    ("5000 × 5000 (200 MB per field), 1000 iterations", lambda r: r["N"] == 5000 and r["iters"] == 1000, "#d62728", "s", False),
    ("5000 × 5000, earlier 10 000-iteration run (timed out after 8 threads)",
     lambda r: r["N"] == 5000 and r["iters"] == 10000, "#d62728", "s", True),
]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 4.8))
for lab, keep, col, m, hollow in series:
    rs = sorted((r for r in runs if keep(r)), key=lambda r: r["threads"])
    x = [r["threads"] for r in rs]
    y = [r["ms_per_sweep"] for r in rs]
    style = dict(marker=m, color=col, mfc="white" if hollow else col, mew=1.5, ms=6,
                 ls=":" if hollow else "-", label=lab)
    ax1.plot(x, y, **style)
    if not hollow:
        ax1.plot([x[0], 64], [y[0], y[0] * x[0] / 64], ls="--", lw=1, color=col, alpha=0.6)
        ax2.plot(x, [y[0] / v for v in y], **style)
ax2.plot([2, 64], [1, 32], "k--", lw=1, label="ideal")

ax1.set_xscale("log", base=2); ax1.set_yscale("log")
ax1.set_xticks([2, 4, 8, 16, 32, 64], ["2", "4", "8", "16", "32", "64"])
ax1.set_xlabel("threads")
ax1.set_ylabel("time per Jacobi sweep [ms]")
ax1.set_title("Time per sweep (dashed: perfect scaling from 2 threads)", fontsize=11)
ax1.legend(fontsize=8.5, frameon=False, loc="lower left")

ax2.set_xlabel("threads")
ax2.set_ylabel("speedup over 2 threads")
ax2.set_title("Speedup", fontsize=11)
ax2.set_xlim(0, 66); ax2.set_ylim(0, 33)
ax2.annotate("1000²: 14× at 64 threads", (64, 2246.62 / 161.052), textcoords="offset points",
             xytext=(-130, 14), fontsize=9, color="#3b6ea5")
ax2.annotate("5000²: stuck near 2× from 8 threads on", (30, 2.0),
             textcoords="offset points", xytext=(-20, 14), fontsize=9, color="#d62728")
ax2.legend(fontsize=8.5, frameon=False, loc="upper left", handles=ax2.get_lines()[:2] + [ax2.get_lines()[-1]])
fig.suptitle("std::for_each(std::execution::par, ...) Jacobi, nvc++ -stdpar=multicore, one Leonardo DCGP node",
             fontsize=11.5)
fig.tight_layout()
fig.savefig(here / "threads.png", dpi=150)
print(f"{len(runs)} runs -> threads.csv, threads.png")
