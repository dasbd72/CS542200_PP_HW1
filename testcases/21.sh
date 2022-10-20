#!/bin/bash
#SBATCH -J 21
#SBATCH -n 24                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 2                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 21.sbatch.out
time srun ./hw1 12347 ./testcases/21.in ./21.out
diff ./testcases/21.out ./21.out