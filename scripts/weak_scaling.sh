#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <program> <initial_problem_size> <n_tries> <thread_list>"
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

# Function to calculate the cubic root
cubic_root() {
    echo "scale=5; e( l($1) / 3 )" | bc -l
}

# Run the program for each number of threads in the list
for THREADS in "${THREAD_ARRAY[@]}"; do
    # Calculate the new problem size
    CUBIC_ROOT=$(cubic_root $THREADS)
    NEW_PROBLEM_SIZE=$(echo "$PROBLEM_SIZE * $CUBIC_ROOT" | bc)
    NEW_PROBLEM_SIZE=${NEW_PROBLEM_SIZE%.*}  # Convert to an integer by truncating the decimal part

    echo "N=$NEW_PROBLEM_SIZE, threads=$THREADS for $N_TRIES times"
    
    for ((i = 1; i <= N_TRIES; i++)); do
        echo "Iteration $i with $THREADS threads"
        ../out/sequential $NEW_PROBLEM_SIZE "../results/weak_scaling_seq.txt"
        ../out/$PROGRAM $NEW_PROBLEM_SIZE $THREADS "../results/weak_scaling.txt"
    done
done
