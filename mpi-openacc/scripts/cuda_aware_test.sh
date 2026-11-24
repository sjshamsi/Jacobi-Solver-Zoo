#!/bin/bash
# Usage, from the mpi-openacc/ folder:  sbatch -A <account> scripts/cuda_aware_test.sh
# Checks that the MPI build is CUDA-aware: rank 0 sends a device buffer straight to rank 1.
#SBATCH --job-name=Check_Cuda_Awareness
#SBATCH --time=00:05:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --cpus-per-task=1
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod
#SBATCH --qos=boost_qos_dbg
#SBATCH --exclusive
#SBATCH --mem=5G
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

set -euo pipefail

module purge
module load hpcx-mpi/2.19
module load nvhpc/24.5
module load cuda/12.6

echo "ompi_info | grep -i cuda output: $( ompi_info | grep -i cuda )"

cp -f "${SLURM_SUBMIT_DIR}/tools/cuda_aware_test.cpp" "${SCRATCH}/"   # submit from the mpi-openacc/ folder
mpic++ -o "${SCRATCH}/cuda_aware_test.x" "${SCRATCH}/cuda_aware_test.cpp" -lcudart -L${CUDA_HOME}/lib64
srun "${SCRATCH}/cuda_aware_test.x" -N 8