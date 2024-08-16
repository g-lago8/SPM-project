#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0  <initial_problem_size> <n_tries> <thread_list>"
    exit 1
fi
PROBLEM_SIZE=$1
N_TRIES=$2
THREAD_LIST=$3

# Check if the provided number of tries is a positive integer
if ! [[ "$N_TRIES" =~ ^[0-9]+$ ]]; then
    echo "Error: The number of tries must be a positive integer."
    exit 1
fi

# Convert thread list to an array
IFS=',' read -r -a THREAD_ARRAY <<< "$THREAD_LIST"


# Run the program for each number of threads in the list
for THREADS in "${THREAD_ARRAY[@]}"; do
    # Calculate the new problem size

    echo "initial N=$PROBLEM_SIZE, threads=$THREADS for $N_TRIES times"
    
    for ((i = 1; i <= N_TRIES; i++)); do
        echo "Iteration $i with $THREADS threads"
        ../out/sequential $PROBLEM_SIZE "../results/weak_scaling_seq.txt"    # Run the sequential version
        ../out/weak_scaling $PROBLEM_SIZE $THREADS "../results/weak_scaling.txt" # Run the parallel version with the specified number of threads
    done
done
