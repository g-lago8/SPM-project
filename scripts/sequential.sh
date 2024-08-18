#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH -o sequential_%j.log
#SBATC -e sequential_%j.err
SIZE=$1
echo "N = $SIZE"
../out/sequential $SIZE
