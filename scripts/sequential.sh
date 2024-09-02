#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH -o ../results/logs/sequential_%j.log
#SBATC -e --/results/errors/sequential_%j.err
SIZE=$1
echo "N = $SIZE"
OUT_FILE=../results/sequential_results.txt
# if the file does not exists, create it with the header
if [ ! -f $OUT_FILE ]; then
    echo "N time" > $OUT_FILE
fi
../out/sequential $SIZE $OUT_FILE
