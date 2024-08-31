#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH -o ../results/logs/sequential_%j.log
#SBATC -e --/results/errors/sequential_%j.err
SIZE=$1
echo "N = $SIZE"
../out/sequential $SIZE
