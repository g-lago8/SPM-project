#SBATCH --job-name=parallel_mpi
#SBATCH --output=../results/mpi_output.txt
#SBATCH --error=../results/mpi_error.txt
#SBATCH --time=00:30:00
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1

mpirun ../out/parallel_mpi