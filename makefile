all: serial parallel

serial:
	gcc -Wall relax_serial.c -o relax_serial

parallel:
	mpicc -Wall relax_parallel.c -o relax_parallel

clean:
	rm -f relax_serial relax_parallel
