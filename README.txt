README

Environment:
--------------------

Built on AQUILA linux system using MPICC and the openMPI library


Modules:
--------------------

module load shared moab torque
module load gcc
module load openmpi/gcc/64

Building:
--------------------

- Run the makefile using 'make'

OR

- Sequential version: gcc -Wall relax_serial.c -o relax_serial
- Parallel   version: mpicc -Wall relax_parallel.c -o relax_parallel



Running:
--------------------

Arguments:
	
	- Array dimension: Integer
	- Process count: Integer
	- Precision: Float
	- Flags:
		
		- '-t': Prints timing information
		- '-p': Prints array results

Serial: ./relax_serial [array dimension] [precision] [flags]

Parallel: mpirun -np [process count] -machinefile &PBS_NODEFILE ./relax_parallel [array dimension] [precision] [flags]