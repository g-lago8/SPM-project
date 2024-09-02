#!/bin/bash
# Set the variables from command-line arguments
#SBATCH --partition=normal
#SBATCH --job-name=parallel_mpi
#SBATCH -o ../results/logs/mpi_omp_%j.log
#SBATCH -e ../results/errors/mpi_omp_%j.err
#SBATCH --time=00:30:00
PROBLEM_SIZE=$1
PPR=$2
#SBATCH --ntasks-per-node=$PPR

srun /bin/hostname

OUT_FILE=../results/mpi_results
# if the file does not exists, create it with the header
if [ ! -f $OUT_FILE]; then
    echo "N mpi_processes time omp_threads" > $OUT_FILE
fi

# Run the MPI program with the specified problem size
mpirun -map-by ppr:$PPR:node --report-bindings ../out/parallel_mpi_omp $PROBLEM_SIZE $OUT_FILE

