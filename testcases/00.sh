#!/bin/bash
#SBATCH -J 00
#SBATCH -n 2                # Number of MPI tasks (i.e. processes)
#SBATCH -c 1                # Number of cores per MPI task
#SBATCH -N 1                # Maximum number of nodes to be allocated
#SBATCH -t 1
#SBATCH -o 00.sbatch.out
time srun ./hw1 15 ./testcases/02.in ./00.out