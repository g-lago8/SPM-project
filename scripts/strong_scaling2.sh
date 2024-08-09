#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <problem_size> <n_tries> <thread_list>"
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
    echo " N=$PROBLEM_SIZE, blocksize=$BLOCKSIZE, on_demand=$ON_DEMAND, threads=$THREADS for $N_TRIES times"
    
    for ((i = 1; i <= N_TRIES; i++)); do
        echo "Iteration $i with $THREADS threads"
        ../out/parallel_ff2 $PROBLEM_SIZE $THREADS
    done
done