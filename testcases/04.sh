#!/bin/bash
#SBATCH -J 04
#SBATCH -n 1                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 1                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 04.sbatch.out
time srun ./hw1 50 ./testcases/04.in ./04.out
diff ./testcases/04.out ./04.out