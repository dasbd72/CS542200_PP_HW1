#!/bin/bash
#SBATCH -J 06
#SBATCH -n 10                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 2                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 06.sbatch.out
time srun ./hw1 65536 ./testcases/06.in ./06.out
diff ./testcases/06.out ./06.out