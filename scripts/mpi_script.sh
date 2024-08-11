#!/bin/bash

# Set the variables from command-line arguments
PROBLEM_SIZE=$1
NODES=$2
NTASKS_PER_NODE=$3

#SBATCH --job-name=parallel_mpi
#SBATCH --output=../results/mpi_output.txt
#SBATCH --error=../results/mpi_error.txt
#SBATCH --time=00:30:00
#SBATCH --nodes=${NODES}
#SBATCH --ntasks-per-node=${NTASKS_PER_NODE}

# Load the necessary module
module load openmpi

# Run the MPI program with the specified problem size
mpirun ../out/parallel_mpi ${PROBLEM_SIZE}
