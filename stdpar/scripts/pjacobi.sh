#!/bin/bash
# Usage, from the stdpar/ folder:  sbatch -A <account> scripts/pjacobi.sh <loops> <N> <iters>
# Builds the NV_REL preset (nvc++ -stdpar=multicore) and sweeps the thread count; timings go to results/logs/.
#SBATCH --job-name=parallel_jacobi
#SBATCH --time=00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=64
#SBATCH --partition=dcgp_usr_prod
##SBATCH --qos=normal
#SBATCH --mem=5G
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err
set -euo pipefail
# set -x


echo " ==== Copying the scripts to the current location ====="

PROJ_DIR_NAME="ParallelJacobi"
PROJ_TEMP_DIR="$SCRATCH/$PROJ_DIR_NAME"
PROJ_DIR="${SLURM_SUBMIT_DIR}"   # submit from the stdpar/ folder
LOG_DIR="${PROJ_DIR}/results/logs"

mkdir -p "$PROJ_TEMP_DIR/build/NV_REL"
cp -r "$PROJ_DIR/src" "$PROJ_TEMP_DIR/"
cp -r "$PROJ_DIR/test" "$PROJ_TEMP_DIR/"
cp -r "$PROJ_DIR/inc" "$PROJ_TEMP_DIR/"
cp "$PROJ_DIR/CMakeLists.txt" "$PROJ_TEMP_DIR/"
cp "$PROJ_DIR/CMakePresets.json" "$PROJ_TEMP_DIR/"


echo " ==== Loading needed modules ====="

module load nvhpc
module load ninja
module load cmake
module load gcc/12.2.0


echo " ==== Building Parallel Jacobi with the NV_REL Preset ====="

cd "$PROJ_TEMP_DIR"
CUSTOM_GCC=/leonardo/prod/spack/06/install/0.22/linux-rhel8-icelake/gcc-8.5.0/gcc-12.2.0-lkcazt4letxjj4s7nlhzryoyivevsatz/bin/gcc cmake --preset NV_REL -S . -B build/NV_REL
cmake --build "$PROJ_TEMP_DIR/build/NV_REL"

mkdir -p "$LOG_DIR"

echo "==== Running jacobi parallel (OpenMP threads sweep) ===="
for T in 2 4 8 12 16 24 36 48 64; do
  echo "---- OMP_NUM_THREADS=${T} ----"
  export OMP_NUM_THREADS="${T}"
  export OMP_PROC_BIND=close
  export OMP_PLACES=cores

  printf "# $T CPUs, " >> log.txt
  srun --cpus-per-task="${T}" build/NV_REL/bin/PJ.x -l "$1" -n "$2" -i "$3"
done

if [[ -f log.txt ]]; then
  mv -f log.txt "$LOG_DIR/threads_${2}x${2}_${SLURM_JOB_ID}.log"
fi


echo "==== Cleanup ===="
cd "$SCRATCH"
rm -rf "$PROJ_TEMP_DIR"


echo "----- Done running jacobi parallel ------"