#!/bin/bash
# Set the variables from command-line arguments
#SBATCH --partition=normal
#SBATCH --job-name=parallel_mpi
#SBATCH -o ../results/logs/mpi_%j.log
#SBATCH -e ../results/errors/mpi_%j.err
#SBATCH --time=00:30:00
PROBLEM_SIZE=$1
PPR=$2
#SBATCH --ntasks-per-node=$PPR

srun /bin/hostname

# Run the MPI program with the specified problem size

mpirun -map-by ppr:$PPR:node --report-bindings ../out/parallel_mpi $PROBLEM_SIZE ../results/mpi_results

