#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <program> <problem_size> <n_tries> <thread_list>"
    exit 1
fi

PROGRAM=$1
PROBLEM_SIZE=$2
N_TRIES=$3
THREAD_LIST=$4

# Check if the provided number of tries is a positive integer
if ! [[ "$N_TRIES" =~ ^[0-9]+$ ]]; then
    echo "Error: The number of tries must be a positive integer."
    exit 1
fi

# Convert thread list to an array
IFS=',' read -r -a THREAD_ARRAY <<< "$THREAD_LIST"

# Run the program for each number of threads in the list
for THREADS in "${THREAD_ARRAY[@]}"; do
    echo " N=$PROBLEM_SIZE, threads=$THREADS for $N_TRIES times"
    
    for ((i = 1; i <= N_TRIES; i++)); do
        echo "Iteration $i with $THREADS threads"
        ../out/$PROGRAM $PROBLEM_SIZE $THREADS ../results/strong_scaling_results.txt
    done
done
