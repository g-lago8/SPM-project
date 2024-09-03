#!/bin/bash
# Run mapping_string.sh on cluster node
if [[ -z "$FF_ROOT" ]]; then
   FF_ROOT=~/fastflow
fi

CURRENT_DIR=$(pwd)
cd $FF_ROOT/ff
yes | srun mapping_string.sh
cd $CURRENT_DIR

# Create the ../results directory if it doesn't exist
if [ ! -d "../results" ]; then
  mkdir ../results
fi

# Create the ../results/logs directory if it doesn't exist
if [ ! -d "../results/logs" ]; then
  mkdir ../results/logs
fi

# Create the ../results/errors directory if it doesn't exist
if [ ! -d "../results/errors" ]; then
  mkdir ../results/errors
fi

echo "Directories checked/created successfully."
