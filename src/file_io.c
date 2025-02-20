#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/file_io.h"

#define DATA_DIRECTORY "data/" // Set to data directory from where the executable is being run
#define INITIAL_CAPACITY 1000	 // Start memory allocation for rows

double **import_file(const char *file_name, size_t *num_rows, size_t *num_cols)
{
	char file_path[200];
	snprintf(file_path, sizeof(file_path), "%s%s", DATA_DIRECTORY, file_name);

	printf("Opening file........(%s)\n", file_path);

	FILE *file = fopen(file_path, "r");
	if (!file)
	{
		printf("Error: Could not open file %s\n", file_path);
		return NULL;
	}

	size_t capacity = INITIAL_CAPACITY;
	double **data = (double **)malloc(capacity * sizeof(double *));
	if (!data)
	{
		printf("Error: Memory allocation failed.\n");
		fclose(file);
		return NULL;
	}

	char *line = malloc(8192 * sizeof(char)); // Large enough buffer for long rows
	if (!line)
	{
		printf("Error: Memory allocation for line buffer failed.\n");
		fclose(file);
		return NULL;
	}

	size_t row = 0;
	*num_cols = 0;

	while (fgets(line, 8192, file))
	{
		if (row >= capacity)
		{
			capacity *= 2;
			double **temp = (double **)realloc(data, capacity * sizeof(double *));
			if (!temp)
			{
				printf("Error: Memory reallocation failed.\n");
				fclose(file);
				return NULL;
			}
			data = temp;
		}

		size_t col = 0;
		char *token = strtok(line, ","); // Split by comma

		// Allocate memory for columns (start with 100, expand dynamically)
		data[row] = (double *)malloc(100 * sizeof(double));
		size_t col_capacity = 100;

		while (token != NULL)
		{
			if (col >= col_capacity)
			{
				col_capacity *= 2;
				double *temp_row = (double *)realloc(data[row], col_capacity * sizeof(double));
				if (!temp_row)
				{
					printf("Error: Column reallocation failed.\n");
					fclose(file);
					return NULL;
				}
				data[row] = temp_row;
			}

			data[row][col] = atof(token);
			token = strtok(NULL, ",");
			col++;
		}

		if (col > *num_cols)
		{
			*num_cols = col; // Update max columns found
		}

		row++;
	}

	fclose(file);
	free(line);

	*num_rows = row;
	printf("File loaded: %zu rows, %zu columns (max detected)\n", *num_rows, *num_cols);
	return data;
}

// Function to print the full dataset in a table format
void print_data(double **data, size_t num_rows, size_t num_cols)
{
	if (!data)
	{
		printf("Error: No data to print.\n");
		return;
	}

	// DEBUG : Print table headers
	num_cols = 5;
	num_rows = 5;
	printf("\nData Table (First %zu Rows, %zu Columns):\n", num_rows, num_cols);
	printf("--------------------------------------------------------------\n");
	printf("| ");
	for (size_t col = 0; col < num_cols; col++)
	{
		printf("Ch%-3zu | ", col + 1); // Label columns as Ch1, Ch2, etc.
	}
	printf("\n--------------------------------------------------------------\n");

	// Print each row
	for (size_t row = 0; row < num_rows; row++)
	{
		printf("| ");
		for (size_t col = 0; col < num_cols; col++)
		{
			printf("%6.2f | ", data[row][col]);
		}
		printf("\n");
	}
	printf("--------------------------------------------------------------\n");
}
