# SPM-project
Project of the Course Parallel and Distributed Systems, University of Pisa
Implementation of a parallel version of a matrix wavefront:
for each diagonal element of a square matrix $M$ ($N\times N$) of double precision
elements, we compute each the element in the superdiagonal $k$  as the dot-product of the same row and the same column of the element.

![Wavefront Picture](image.png)
## Usage

To compile the code, run the following command in the folder `src`:
```bash
make all
```
Executables will be in the folder `out`.

Brief explanation of the files in the `src` folder:

- `sequential.cpp`: Sequential wavefront

- `check_correcness_ff.cpp`: check the correctness of FastFlow the wavefront computation. 

- `compare_sequential.cpp`: compares different sequential version with increasing optimization.

- `parallel_ff.cpp`: and `parallel_ff_block_cyclic`: two different FastFlow implementations (see the report).

- `parallel_omp.cpp`: Parallel version with just OpenMP (not present in the report).

- `weak_scaling.cpp`: simply runs the sequential implementation and that in `parallel_ff.cpp`, but on a matrix of size $N\times \sqrt[3]{nworkers} $, where $N$ is chosen by the user. This is a (pretty dumb) way I found to write a simple weak scaling test, without having to do floating point operations in shell scripts, which requires `bc` ( installed in the fontend node but not in the other nodes).

Additionally, with the command `make parallel_mpi USE_OPENMP=1` you can compile the MPI version with OpenMP support. The number of threads is set by the environment variable `OMP_NUM_THREADS`.

### Scripts 
in the folder `scripts` there are some scripts to run the code on the cluster. The scripts are:

- `strong_scaling.sh`: runs the code on the cluster with a fixed matrix size and increasing number of workers.
Usage: `./strong_scaling.sh <matrix_size> <n_repetitions> <thread_list>`.  thread_list is to be given separated by commas.
Results will be in the file `results/strong_scaling_results.txt`.
- `weak_scaling.sh`: runs the code on the cluster with a matrix size that increases with the number of workers.
Usage: `./weak_scaling.sh <initial_matrix_size> <n_repetitions> <thread_list>`. `initial_matrix_size` is the size of the matrix for 1 worker, and the size of the matrix for n workers is $N\times \sqrt[3]{nworkers} $, where $N$ is the i`nitial_matrix_size`.
Results will be in the file `results/weak_scaling_results.txt`.

- `sequential.sh`: runs the sequential code on the cluster with a fixed matrix size. Usage: `./sequential.sh <matrix_size>.` Results will be in the file `results/sequential_results.txt`.

- `mpi_script.sh`: Runs the MPI code on the cluster with a fixed matrix size the given number of workers. Usage: 
`sbatch --nodes=N mpi_script.sh <matrix_size> <processes_per_node>`. 
- `mpi_script_omp.sh`: Runs the MPI code on the cluster with a fixed matrix size the given number of workers. Usage:
`sbatch --nodes=N mpi_script_omp.sh <matrix_size> <processes_per_node>`. 