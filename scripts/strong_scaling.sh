#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 6 ]; then
    echo "Usage: $0 <program> <problem_size> <blocksize> <on_demand> <n_tries> <thread_list>"
    exit 1
fi


PROGRAM =$1
PROBLEM_SIZE=$2
BLOCKSIZE=$3
ON_DEMAND=$4
N_TRIES=$5
THREAD_LIST=$6

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
        ../out/strong_scaling_new $PROBLEM_SIZE $THREADS $BLOCKSIZE $ON_DEMAND 
    done
done
