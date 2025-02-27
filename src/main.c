#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../inc/file_io.h"

// Maximum channel number

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
		get_file_name(file_name, sizeof(file_name));
		// Ask for channel number and validate input
		get_channel_num(&channel_num);
	}
	else
	{
		// Handle command line arguments
		// Safely copy arguments to prevent buffer overflow
		strncpy(file_name, argv[1], sizeof(file_name) - 1);
		file_name[sizeof(file_name) - 1] = '\0'; // Ensure null termination
		channel_num = atoi(argv[2]);						 // Convert argument to integer

		// Validate file name & channel number
		if (validate_file_name(file_name))
			return 1; // Exit with error
		if (validate_channel_num(channel_num))
			return 1; // Exit with error
	}

	printf("\n");
	printf("----------------------------------------\n");
	printf("Reading from file: %s\n", file_name);
	printf("Channel number: %d\n", channel_num);
	printf("----------------------------------------\n");
	printf("\n");

	// Read data from file
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