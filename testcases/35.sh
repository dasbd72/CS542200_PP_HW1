#!/bin/bash
#SBATCH -J 35
#SBATCH -n 12                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 1                # Maximum number of nodes to be allocated
#SBATCH -t 3
#SBATCH -o 35.sbatch.out
time srun ./hw1 536869888 ./testcases/35.in ./35.out
diff ./testcases/35.out ./35.out
rm ./35.out