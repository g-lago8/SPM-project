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
OUT_FILE=../results/mpi_results

# if the file does not exists, create it with the header
if [ ! -f $OUT_FILE ]; then
    echo "N mpi_processes time" > $OUT_FILE
fi

# Run the MPI program with the specified problem size and number of processes
mpirun -map-by ppr:$PPR:node --report-bindings ../out/parallel_mpi $PROBLEM_SIZE ../results/mpi_results

