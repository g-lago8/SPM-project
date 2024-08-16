#!/bin/bash
# Set the variables from command-line arguments
#SBATCH --partition=normal
#SBATCH --job-name=parallel_mpi
#SBATCH -o ../results/%j.log
#SBATCH -e ../results/%j.err
#SBATCH --time=00:30:00
#SBATCH --ntasks-per-node=1
PROBLEM_SIZE=$1

srun /bin/hostname

# Run the MPI program with the specified problem size

mpirun -map-by ppr:1:node --report-bindings ../out/parallel_mpi $PROBLEM_SIZE
