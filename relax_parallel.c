#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

// Store the current time in a timeval struct
void record_time(struct timeval* time)
{
	gettimeofday(time, NULL);
}

// Calculate difference in time of two timeval structs
double calculate_time_passed(struct timeval start, struct timeval end)
{
	double time = 0.0;
	
	time += (double) (end.tv_sec - start.tv_sec);
	time += ((double) (end.tv_usec - start.tv_usec)) / 1000000;
	
	return time;
}

// Compares two arrays to see if they differ by more than the precision
int compare(double** input, double** output, int dimension, int sub_dimension, double precision)
{
	int outer;
	int inner;
	double difference;

	for (outer = 1; outer < sub_dimension; outer++)
	{
		for (inner = 1; inner < dimension - 1; inner++)
		{
			difference = fabs(output[outer][inner] - input[outer][inner]);

			if (difference > precision)
			{
				return 0;
			}
		}
	}

	return 1;
}

// Calculates the relaxed values for cells in an array
void relax(double** input, double** output, int dimension, int sub_dimension)
{
	double above;
	double below;
	double left;
	double right;
	double result;
	int cell_x;
	int cell_y;

	// Calculate relaxed values for each cell in the output array
	for (cell_y = 1; cell_y < sub_dimension - 1; cell_y++)
	{
		for (cell_x = 1; cell_x < dimension - 1; cell_x++) 
		{
			// Retrieve adjacent cell values
			above = input[cell_y - 1][cell_x];
			below = input[cell_y + 1][cell_x];
			left = input[cell_y][cell_x - 1];
			right = input[cell_y][cell_x + 1];

			// Calculate average and store in output cell
			result = (above + below + left + right) / 4.0;
			output[cell_y][cell_x] = result;
		}
	}
}

// Initialise MPI and store rank / number of processes
void initialise(int* argc, char*** argv)
{
	int init_check = MPI_Init(argc, argv);
	if (init_check != MPI_SUCCESS)
	{
		fprintf(stderr, "Error initialising MPI\n");
		MPI_Abort(MPI_COMM_WORLD, init_check);
	}
}

// Calculate the rows a process is responsbile for
void calculate_rows(int* lower_row, int* upper_row, int dimension)
{
	int sub_dimension;
	int overflow;
	int modifier;
	int temp_lower_row;
	int temp_upper_row;
	int index;
	int rank;
	int process_count;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);

	// If there are fewer processes than acrtive rows
	if (process_count > dimension)
	{
		sub_dimension = 1;
		overflow = 0;
	}
	else
	{
		sub_dimension = ((dimension - 2) / process_count);
		overflow = (dimension - 2) % process_count;
	}
	
	// Calculate the modifier to correctly adjust row assignments
	if (sub_dimension == 0)
	{
		modifier = 0;
	}
	else
	{
		modifier = 1;
	}
	
	temp_lower_row = 1;
	temp_upper_row = temp_lower_row + sub_dimension - modifier;
	
	// Loop through until the rows for this process have been calculated
	for (index = 0; index < process_count; ++index)
	{
		if (overflow > 0)
		{
			temp_upper_row++;
			overflow--;
		}
		
		if (index == rank)
		{
			*lower_row = temp_lower_row - 1;
			*upper_row = temp_upper_row + 1;
			break;
		}

		temp_lower_row = temp_upper_row + 1;
		temp_upper_row = temp_lower_row + sub_dimension - modifier;
	}
}

// Function for printing the final result
void print(double** result, int width, int height)
{
	int count;
	int temp;
	int print_outer;
	int print_inner;
	int rank;
	int process_count;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	
	// Print the process arrays in order
	for (count = 0; count < process_count; count++)
	{
		if (rank == count)
		{
			// If first process print first row of the array
			if (rank == 0)
			{
				for (temp = 0; temp < width; temp++)
				{
					printf("%f ", result[0][temp]);
				}
				printf("\n");
			}
		
			// Print center rows of the array
			for (print_outer = 1; print_outer < height - 1; print_outer++)
			{
				for (print_inner = 0; print_inner < width; print_inner++)
				{
					printf("%f ", result[print_outer][print_inner]);
				}
				printf("\n");
			}
			
			// If last process print last row of the array
			if (rank == process_count - 1)
			{
				for (temp = 0; temp < width; temp++)
				{
					printf("%f ", result[height - 1][temp]);
				}
				printf("\n");
			}
		}
		
		// Wait at barrier to ensure results are printed in order
		MPI_Barrier(MPI_COMM_WORLD);
	}
}

int main(int argc, char* argv[])
{
	// Timing variables
	struct timeval message_start;
	struct timeval message_end;
	double message_time = 0.0;
	struct timeval parallel_start;
	struct timeval parallel_end;
	double parallel_time = 0.0;	
	
	int rank;
	int process_count;
	int lower_row;
	int upper_row;
	int direction = 1;
	int running = 1;
	int index;
	int self_finished = 0;
	int all_finished = 0;
	MPI_Status status;
	
	// Array variables
	int width;
	int height;
	double** left_array = NULL;
	double** right_array = NULL;
	double** input = NULL;
	double** output = NULL;
	
	// Parse command line arguments
	int dimension = strtol(argv[1], NULL, 10);
	double precision = strtod(argv[2], NULL);
	int time_flag = 0;
	int print_flag = 0;
	
	// Check for flag arguments
	for(index = 3; index < argc; index++)
	{
		// Check for timing flag
		if (strcmp("-t", argv[index]) == 0)
		{
			time_flag = 1;
		}
		// Check for printing flag
		else if (strcmp("-p", argv[index]) == 0)
		{
			print_flag = 1;
		}
	}
	
	// Initialise MPI and store rank / number of processes
	initialise(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	
	if (time_flag == 1 && rank == 0)
	{
		record_time(&parallel_start);
	}
	
	// Each process calculates which rows of the array it should work on
	calculate_rows(&lower_row, &upper_row, dimension);
	
	width = dimension;
	height = (upper_row - lower_row) + 1;

	// Allocate array memory and set default values
	left_array = malloc(height * sizeof(double*));
	right_array = malloc(height * sizeof(double*));
	
	for (index = 0; index < height; index++)
	{
		left_array[index] = malloc(width * sizeof(double));
		right_array[index] = malloc(width * sizeof(double));
	}
	
	int outer;
	int inner;
	int row = lower_row;

	for (outer = 0; outer < height; outer++)
	{
		for(inner = 0; inner < width; inner++)
		{
			if (row == 0 || inner == 0)
			{
				left_array[outer][inner] = 1.0;
				right_array[outer][inner] = 1.0;
			}
			else
			{
				left_array[outer][inner] = 0.0;
				right_array[outer][inner] = 0.0;
			}
		}
		row++;
	}
	
	// Parallel relaxing of arrays
	while (running == 1)
	{
		// Calculate which direction to use the arrays in
		if (direction == 1)
		{
			input = left_array;
			output = right_array;
		}
		else
		{
			input = right_array;
			output = left_array;
		}
		
		relax(input, output, width, height);
		
		if (time_flag == 1 && rank == 0)
		{
			record_time(&parallel_end);
			parallel_time += calculate_time_passed(parallel_start, parallel_end);
			
			record_time(&message_start);
		}

		// Send lowest row to lower rank
		if (rank != 0)
		{		
			MPI_Send(&output[1][0], width, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD);
		}
		
		// Read lowest row from higher rank
		if (rank != process_count - 1)
		{
			MPI_Recv(&output[height - 1][0], width, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &status);
		}
		
		// Send highest row to higher rank
		if (rank != process_count - 1)
		{
			MPI_Send(&output[height - 2][0], width, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD);
		}
		
		// Read highest row from lower rank
		if (rank != 0)
		{
			MPI_Recv(&output[0][0], width, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &status);
		}
		
		if (time_flag == 1 && rank == 0)
		{
			record_time(&message_end);
			message_time += calculate_time_passed(message_start, message_end);
			
			record_time(&parallel_start);
		}
		
		// Calculate if this process has finished
		self_finished = compare(input, output, width, height, precision);
		
		if (time_flag == 1 && rank == 0)
		{
			record_time(&parallel_end);
			parallel_time += calculate_time_passed(parallel_start, parallel_end);
			
			record_time(&message_start);
		}
		
		// Reduce all finished values
		MPI_Reduce(&self_finished, &all_finished, 1, MPI_INT, MPI_LAND, 0, MPI_COMM_WORLD);
		
		// Broadcast this reduced value to all processes
		MPI_Bcast(&all_finished, 1, MPI_INT, 0, MPI_COMM_WORLD);
		
		if (time_flag == 1 && rank == 0)
		{
			record_time(&message_end);
			message_time += calculate_time_passed(message_start, message_end);
			
			record_time(&parallel_start);
		}

		// If all processes have finished then exit
		if (all_finished == 1)
		{
			running = 0;
		}
		else
		{
			// Flip direction arrays are used in
			direction = -direction;
		}
	}
		
	if (time_flag == 1 && rank == 0)
	{
		record_time(&parallel_end);
		parallel_time += calculate_time_passed(parallel_start, parallel_end);
	
		printf("Parallel time: %f\n", parallel_time);
		printf("Message time: %f\n", message_time);
	}
	
	if (print_flag == 1)
	{
		print(output, width, height);
	}
	
	MPI_Finalize();
	return 0;
}