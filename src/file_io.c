#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../inc/file_io.h"

const char *FILE_EXTENSIONS[] = {".csv", ".bin"};

double **import_file(const char *file_name, size_t *num_rows, size_t *num_cols)
{

	char file_path[200];
	snprintf(file_path, sizeof(file_path), "%s%s", DATA_DIRECTORY, file_name);

	printf("Opening file........(%s)\n", file_path);

	FILE *file = fopen(file_path, "r");
	if (!file)
	{
		printf("Error: Could not open file %s\n", file_name);
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
	printf("File loaded: %zu row(s), %zu column(s)\n", *num_rows, *num_cols);
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

int verify_result(double **data, size_t data_num_rows, size_t data_num_cols, const char *file)
{
	size_t verify_num_rows = 0; // Variable to store the number of rows read
	size_t verify_num_cols = 0; // Variable to store the number of columns read
	double **verify_data = import_file(file, &verify_num_rows, &verify_num_cols);

	if (!verify_data)
	{
		printf("Error: Could not verify result.\n\n");
		return 1;
	}

	// compare rows and columns of data and verify_data
	if (verify_num_rows != data_num_rows)
	{
		printf("Error: Number of rows do not match.\n\n");
		printf("verify_num_rows: %zu\n", verify_num_rows);
		printf("data_num_rows: %zu\n", data_num_rows);
		return 1;
	}

	printf("Row match : %zu row(s)\n", verify_num_rows);

	if (verify_num_cols != data_num_cols)
	{
		printf("Error: Number of columns do not match.\n\n");
		printf("verify_num_cols: %zu\n", verify_num_cols);
		printf("data_num_cols: %zu\n", data_num_cols);
		return 1;
	}

	printf("Column match : %zu column(s)\n", verify_num_cols);

	// compare each row of data and verify_data
	int count = 0;
	for (size_t row = 0; row < data_num_rows; row++)
	{
		for (size_t col = 0; col < data_num_cols; col++)
		{
			if (data[row][col] != verify_data[row][col])
			{
				printf("Error: Data mismatch at row %zu, column %zu.\n\n", row, col);
				// print the value of data
				printf("data: %f\n", data[row][col]);
				// print the value of verify_data
				printf("verify_data: %f\n", verify_data[row][col]);

				return 1;
			}
			count++;
		}
	}
	printf("Data match: %d value(s) checked\n\n", count);

	return 0; // Files match
}

void get_file_name(char *file_name, size_t file_name_size)
{
	char input_buffer[100]; // Buffer for user input

	while (1)
	{
		printf("\nEnter the file name: ");
		if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL)
		{
			printf("Error reading input.\n");
			continue;
		}

		// Remove newline character from input
		input_buffer[strcspn(input_buffer, "\n")] = '\0';

		if (strcmp(input_buffer, "0") == 0 || input_buffer[0] == '\0')
		{
			strncpy(file_name, DEFAULT_FILE, file_name_size - 1);
			file_name[file_name_size - 1] = '\0'; // Ensure null termination
			printf("Opening default file: %s\n", DEFAULT_FILE);
			break;
		}
		else
		{
			// Validate file name
			if (validate_file_name(input_buffer))
			{
				continue;
			}

			strncpy(file_name, input_buffer, file_name_size - 1);
			file_name[file_name_size - 1] = '\0'; // Ensure null termination
			printf("Opening file: %s\n", file_name);
			break;
		}
	}
}

// fucntion to validate file name from command line args
int validate_file_name(const char *file_name)
{
	// Check if file name ends with valid file extensions
	char *ext = strrchr(file_name, '.'); // Get last occurrence of '.'
	int valid_ext = 0;
	for (int i = 0; i < sizeof(FILE_EXTENSIONS) / sizeof(FILE_EXTENSIONS[0]); i++)
	{
		if (ext && strcmp(ext, FILE_EXTENSIONS[i]) == 0)
		{
			valid_ext = 1;
			return 0;
		}
	}
	printf("Error: File must end with .csv or .bin.\n");
	return 1; // Exit with error
}

void get_channel_num(int *channel_num)
{
	// Ask for channel number and validate input
	while (1)
	{
		printf("\nEnter the channel number: ");

		// Check if channel number is an integer
		if (scanf("%d", channel_num) != 1) // Validate integer input
		{
			printf("Error: Invalid input. Please enter an integer.\n");
			while (getchar() != '\n')
				; // Clear input buffer
			continue;
		}
		// Check if channel number is within the range
		if (validate_channel_num(*channel_num, MAX_CHANNEL))
		{
			continue; // Retry input
		}
		break;
	}
}

int validate_channel_num(int channel_num, int max_channel)
{
	if (channel_num <= 0 || channel_num > max_channel)
	{
		printf("Error: Channel number must be between 1 and %d.\n", max_channel);
		return 1; // Exit with error
	}
	return 0;
}
