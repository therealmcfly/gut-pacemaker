#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.c"

#define PRINT_ROWS 10

// constants
#define MAX_CHANNEL 250														// Maximum channel number
const char *FILE_EXTENSIONS[] = {".csv", ".bin"}; // File extentions allowed

int main(int argc, char *argv[])
{

	char file_name[100];	// Buffer for file name
	int channel_num;			// Buffer for channel number
	size_t num_rows = 0;	// Variable to store the number of rows read
	size_t num_cols = 0;	// Variable to store the number of columns read
	double *channel_data; // Pointer to dynamically allocated array

	// If no arguments are passed, ask for user input
	if (argc != 3)
	{
		// Ask for filename and validate input
		while (1)
		{
			printf("Enter the file name: ");
			scanf("%99s", file_name); // Store user input in buffer (avoid overflow)

			// Check if file name is empty
			if (file_name[0] == '\0')
			{
				printf("Error: File name cannot be empty.\n");
				continue;
			}

			// Check if file name ends with const file extentions
			char *ext = strrchr(file_name, '.'); // Get last occurrence of '.'
			int valid_ext = 0;
			for (int i = 0; i < sizeof(FILE_EXTENSIONS) / sizeof(FILE_EXTENSIONS[0]); i++)
			{
				if (ext && strcmp(ext, FILE_EXTENSIONS[i]) == 0)
				{
					valid_ext = 1;
					break;
				}
			}
			break;
		}

		// Ask for channel number and validate input
		while (1)
		{
			printf("Enter the channel number: ");

			// Check if channel number is an integer
			if (scanf("%d", &channel_num) != 1) // Validate integer input
			{
				printf("Error: Invalid input. Please enter an integer.\n");
				while (getchar() != '\n')
					; // Clear input buffer
				continue;
			}
			// Check if channel number is within the range
			if (channel_num <= 0 || channel_num > MAX_CHANNEL)
			{
				printf("Error: Channel number must be between 1 and %d\n", MAX_CHANNEL);
				continue;
			}
			break;
		}
	}
	else
	{
		// Safely copy arguments to prevent buffer overflow
		strncpy(file_name, argv[1], sizeof(file_name) - 1);
		file_name[sizeof(file_name) - 1] = '\0'; // Ensure null termination

		channel_num = atoi(argv[2]); // Convert argument to integer

		// Validate file name from command line args
		char *ext = strrchr(file_name, '.'); // Get last occurrence of '.'
		if (!ext || (strcmp(ext, FILE_EXTENSIONS[0]) != 0 && strcmp(ext, FILE_EXTENSIONS[1]) != 0))
		{
			printf("Error: File must end with .csv or .bin.\n");
			return 1; // Exit with error
		}

		// Validate channel number from command line args
		if (channel_num <= 0 || channel_num > MAX_CHANNEL)
		{
			printf("Error: Channel number must be between 1 and %d.\n", MAX_CHANNEL);
			return 1; // Exit with error
		}
	}

	printf("\n");
	printf("----------------------------------------\n");
	printf("Reading from file: %s\n", file_name);
	printf("Channel number: %d\n", channel_num);
	printf("----------------------------------------\n");
	printf("\n");

	// import file
	double **data = import_file(file_name, &num_rows, &num_cols);
	if (!data)
	{
		printf("Error: Failed to load data.\n");
		return 1;
	}

	// verify result
	if (verify_result(data, num_rows, num_cols, "MATLAB Model/verify_importfile.csv"))
	{
		return 1;
	}

	return 0;
}