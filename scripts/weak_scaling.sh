#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH -o ../results/logs/ff_ws_%j.log
#SBATCH -e ../results/errors/ff_ws_%j.err

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0  <initial_problem_size> <n_tries> <thread_list>"
    exit 1
fi
INITIAL_PROBLEM_SIZE=$1
N_TRIES=$2
THREAD_LIST=$3

# Check if the provided number of tries is a positive integer
if ! [[ "$N_TRIES" =~ ^[0-9]+$ ]]; then
    echo "Error: The number of tries must be a positive integer."
    exit 1
fi

OUT_FILE=../results/weak_scaling_results.txt
# if the file does not exists, create it with the header
if [ ! -f $OUT_FILE ]; then
    echo "N n_workers time" > $OUT_FILE
fi


# Convert thread list to an array
IFS=',' read -r -a THREAD_ARRAY <<< "$THREAD_LIST"


# Run the program for each number of threads in the list
for THREADS in "${THREAD_ARRAY[@]}"; do
    # Calculate the new problem size

    echo "initial N=$INITIAL_PROBLEM_SIZE, threads=$THREADS for $N_TRIES times"
    
    for ((i = 1; i <= N_TRIES; i++)); do
        echo "Iteration $i with $THREADS threads"
        ../out/weak_scaling $INITIAL_PROBLEM_SIZE $THREADS $OUT_FILE # Run the parallel version with the specified number of threads
    done
done
