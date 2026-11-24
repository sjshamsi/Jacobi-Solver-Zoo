from mpi4py import MPI
from collections import defaultdict


class MPITimer:
    def __init__(self):
        # Total accumulated time per event name (per-rank)
        self.times = defaultdict(float)
        # Start times for currently-running events
        self._t0 = {}

    def start(self, name: str):
        # Re-starting an event name is allowed; it will accumulate when stopped.
        # (If it's already running, we overwrite the start time.)
        self._t0[name] = MPI.Wtime()

    def stop(self, name: str):
        t0 = self._t0.get(name, None)
        if t0 is None:
            raise KeyError(f"MPITimer.stop('{name}') called without a matching start('{name}')")
        self.times[name] += MPI.Wtime() - t0
        del self._t0[name]

    def report(self, comm=MPI.COMM_WORLD):
        """
        Print, for each event, the MAX of the total accumulated time across ranks.
        """
        rank = comm.Get_rank()

        # Ensure consistent ordering across ranks (helps readability)
        event_names = sorted(self.times.keys())

        for name in event_names:
            t_local = self.times[name]
            t_max = comm.reduce(t_local, op=MPI.MAX, root=0)
            if rank == 0:
                print(f"{name:20s}  max={t_max:.6f}s")


if __name__ == "__main__":
    import time

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()

    timer = MPITimer()

    # --------------------------------
    # Test 1: same event, multiple intervals
    # --------------------------------
    timer.start("compute")
    time.sleep(0.1 * (rank + 1))   # rank-dependent time
    timer.stop("compute")

    timer.start("compute")
    time.sleep(0.05 * (rank + 1))
    timer.stop("compute")

    # --------------------------------
    # Test 2: another independent event
    # --------------------------------
    for i in range(3):
        timer.start("communication")
        time.sleep(2 * (rank + 1))
        timer.stop("communication")

    # Optional synchronization before reporting
    comm.Barrier()

    if rank == 0:
        print("\n=== MPI Timer Report (MAX over ranks) ===")

    timer.report(comm)