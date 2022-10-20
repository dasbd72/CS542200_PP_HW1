#!/bin/bash
#SBATCH -J 07
#SBATCH -n 3                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 3                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 07.sbatch.out
time srun ./hw1 12345 ./testcases/07.in ./07.out
diff ./testcases/07.out ./07.out