#!/bin/bash
#SBATCH -J 02
#SBATCH -n 15                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 3                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 02.sbatch.out
time srun ./hw1 15 ./testcases/02.in ./02.out
diff ./testcases/02.out ./02.out