#!/bin/bash
#SBATCH -J 33
#SBATCH -n 12                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 1                # Maximum number of nodes to be allocated
#SBATCH -t 3
#SBATCH -o 33.sbatch.out
time srun ./hw1 536869888 ./testcases/33.in ./33.out
diff ./testcases/33.out ./33.out
rm ./33.out