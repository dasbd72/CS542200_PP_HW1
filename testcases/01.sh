#!/bin/bash
#SBATCH -J 01
#SBATCH -n 4                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 2                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 01.sbatch.out
time srun ./hw1 4 ./testcases/01.in ./01.out
diff ./testcases/01.out ./01.out