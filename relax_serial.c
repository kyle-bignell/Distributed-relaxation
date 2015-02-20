#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void print_array(double** array, int dimension)
{	
	int outer;
	int inner;
	for (outer = 0; outer < dimension; outer++)
	{
		for (inner = 0; inner < dimension; inner++)
		{
			printf("%f ", array[outer][inner]);
		}
		printf("\n");
	}
}

void recordTime(struct timeval* time)
{
	gettimeofday(time, NULL);
}

double calculateTimePassed(struct timeval start, struct timeval end)
{
	double time = 0.0;
	
	time += (double) (end.tv_sec - start.tv_sec);
	time += ((double) (end.tv_usec - start.tv_usec)) / 1000000;
	
	return time;
}

// Calculates the relaxed values for cells in an array
void relax(double** input, double** output, int dimension)
{
	int cell_y;
	int cell_x;
	double above;
	double below;
	double left;
	double right;
	double result;
	
	for (cell_y = 1; cell_y < dimension - 1; cell_y++)
	{
		for (cell_x = 1; cell_x < dimension - 1; cell_x++) 
		{
			above = input[cell_y - 1][cell_x];
			below = input[cell_y + 1][cell_x];
			left = input[cell_y][cell_x - 1];
			right = input[cell_y][cell_x + 1];

			result = (above + below + left + right) / 4.0;
			output[cell_y][cell_x] = result;
		}
	}
}

// Compares two arrays to see if they differ by more than the precision
int compare(double** input, double ** output, int dimension, double precision)
{
	int outer;
	int inner;
	double difference;

	for (outer = 1; outer < dimension - 1; outer++)
	{
		for (inner = 1; inner < dimension - 1; inner++)
		{
			difference = output[outer][inner] - input[outer][inner];

			if (difference > precision)
			{
				return 0;
			}
		}
	}

	return 1;
}

void initialiseArrayValues(double** array, int dimension)
{
	int outer;
	int inner;

	for (outer = 0; outer < dimension; outer++)
	{
		for(inner = 0; inner < dimension; inner++)
		{
			if (outer == 0 || inner == 0)
			{
				array[outer][inner] = 1;
			}
			else
			{
				array[outer][inner] = 0;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	int index;
	struct timeval serial_start;
	struct timeval serial_end;
	double serial_time = 0.0;
	
	// Arguments	
	int dimension = strtol(argv[1], NULL, 10);
	double precision = strtod(argv[2], NULL);
	int time_flag = 0;
	int print_flag = 0;
	
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
	
	double** left_array;
	double** right_array;
	double** input;
	double** output;
	int direction = 1;
	int complete = 0;

	
	if(time_flag)
	{
		recordTime(&serial_start);
	}
	
	// Allocate array memory
	left_array = (double**) malloc(dimension * sizeof(double*));
	right_array = (double**) malloc(dimension * sizeof(double*));
	for (index = 0; index < dimension; index++)
	{
		left_array[index] = (double*) malloc(dimension * sizeof(double*));
		right_array[index] = (double*) malloc(dimension * sizeof(double*));
	}
	
	initialiseArrayValues(left_array, dimension);
	initialiseArrayValues(right_array, dimension);
		
	while (complete == 0)
	{
		// Calculate the direction to use the arrays in
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
		
		relax(input, output, dimension);
		
		// Compare the differences in the arrays
		if (compare(input, output, dimension, precision))
		{
			complete = 1;
		}
		else
		{
			direction =  -direction;	
		}
	}
	
	if (time_flag)
	{
		recordTime(&serial_end);
		serial_time += calculateTimePassed(serial_start, serial_end);
	
		printf("Serial time: %f\n", serial_time);
	}
	
	if (print_flag)
	{
		if (direction == 1)
		{
			print_array(right_array, dimension);
		}
		else
		{
			print_array(left_array, dimension);
		}
	}
	
	// Clean up array memory
	for (index = 0; index < dimension; index++)
	{
		free(left_array[index]);
		free(right_array[index]);
	}

	free(left_array);
	free(right_array);

	return 0;
}
