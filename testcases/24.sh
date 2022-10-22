#!/bin/bash
#SBATCH -J 24
#SBATCH -n 24                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 2                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 24.sbatch.out
time srun ./hw1 400009 ./testcases/24.in ./24.out
diff ./testcases/24.out ./24.out
rm ./24.out