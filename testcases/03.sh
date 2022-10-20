#!/bin/bash
#SBATCH -J 03
#SBATCH -n 28                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 4                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 03.sbatch.out
time srun ./hw1 21 ./testcases/03.in ./03.out
diff ./testcases/03.out ./03.out