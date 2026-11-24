#!/bin/bash
# Usage, from the mpi-openacc/ folder:  sbatch -A <account> scripts/gpu_jacobi.sh
# Builds with the Makefile and runs every bin/*.x under Nsight Systems (MPI, CUDA, OpenACC and NVTX traces).
#SBATCH --job-name=GPU_Jacobi
#SBATCH --time=00:10:00
#SBATCH --nodes=16
#SBATCH --ntasks-per-node=4
#SBATCH --cpus-per-task=8
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod
#SBATCH --qos=normal
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

set -euo pipefail

echo " ==== Loading modules... ===== "

module load hpcx-mpi/2.19
module load nvhpc/24.5
module load cuda/12.6

echo " ==== Building Project... ===== "
WORK_ROOT=$(mktemp -d "${SCRATCH}/XXXXXX")
mkdir -p "${WORK_ROOT}"
cp -rf "${SLURM_SUBMIT_DIR}" "${WORK_ROOT}/mpi-openacc"   # submit from the mpi-openacc/ folder

WORK_DIR="${WORK_ROOT}/mpi-openacc"
make -C "${WORK_DIR}" all

for f in "${WORK_DIR}"/bin/*.x; do
    echo -e "\n\n$(basename "$f") output:"
    srun nsys profile \
    --nic-metrics=true \
    --trace=mpi,cuda,openacc,nvtx \
    -o "${SLURM_SUBMIT_DIR}/nsys_%q{SLURM_JOB_ID}_N%q{SLURM_JOB_NUM_NODES}_%q{SLURM_PROCID}" "$f"
    echo
done

rm -rf "${WORK_ROOT}"
echo "==== Cleanup Done! ==== "