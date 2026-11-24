#!/bin/bash
# Usage, from the python/ folder:  sbatch -A <account> job.slurm
#SBATCH --job-name=Python_jacobi_MPIonly_cupy
#SBATCH --time=00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=32
#SBATCH --gres=gpu:1
#SBATCH --partition=boost_usr_prod
#SBATCH --qos=boost_qos_dbg
#SBATCH --mem=0
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err


SCRIPT_DIR_NAME=Day3_Jacobi
SCRIPT_DIR_PATH=${SLURM_SUBMIT_DIR}   # submit from the python/ folder
TEMP_WORK_DIR=$(mktemp -d)

cp -rf ${SCRIPT_DIR_PATH} ${TEMP_WORK_DIR}/
WORK_DIR=${TEMP_WORK_DIR}/${SCRIPT_DIR_NAME}

module purge
module load nvhpc
module load hpcx-mpi
module load cuda

source ${HOME}/miniconda3/etc/profile.d/conda.sh
conda activate numba

export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}
export OMP_PROC_BIND=close
export OMP_PLACES=cores
export NUMBA_NUM_THREADS=${SLURM_CPUS_PER_TASK}

srun python ${WORK_DIR}/main.py
