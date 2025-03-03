#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../inc/file_io.h"
#include "../inc/pre_processing.h"
#include "../inc/result_check.h"

#define MATLAB_DIRECTORY "MATLAB Model/"
#define DEBUG 1

int main(int argc, char *argv[])
{
	char file_name[100];		 // Buffer for file name
	int channel_num;				 // Buffer for channel number
	size_t num_rows = 0;		 // Variable to store the number of rows read
	size_t num_cols = 0;		 // Variable to store the number of columns read
	size_t ver_num_rows = 0; // Variable to store the number of rows read
	size_t ver_num_cols = 0; // Variable to store the number of columns read
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
		if (validate_channel_num(channel_num, MAX_CHANNEL))
			return 1; // Exit with error
	}

	printf("\n");
	printf("----------------------------------------\n");
	printf("Reading from file: %s\n", file_name);
	printf("Channel number: %d\n", channel_num);
	printf("----------------------------------------\n");

	// Read data from file
	printf("\nReading data from file...\n");
	float **data = import_file(file_name, &num_rows, &num_cols);
	if (!data)
	{
		printf("Error: Failed to load data.\n");
		return 1;
	}
	// Verify channel data with MATLAB output
	// if (verify_result(channel_data, num_rows, num_cols, "ver_chdata_%s_ch%d", *strtok(file_name, "."), channel_num))
	// {
	// 	printf("Error: Verification failed.\n");
	// 	return 1;
	// }

	/* ---------------------------------------------------------------------- */
	/* -------------------------- Pre Processing ---------------------------- */
	/* ---------------------------------------------------------------------- */
	// code below here is likely to be used in embedded systems. lib usage may be restricted, memory and resources may be limited

	/* ------------------------ Channel Data Retrieval --------------------------- */
	// Get the channel data
	float *channel_data = get_ch_signal(data, num_rows, num_cols, channel_num);
	// // DEBUG: Print the first 10 samples of the channel
	// for (size_t i = 0; i < 10 && i < num_rows; i++)
	// {
	// 	printf("channel[%zu] = %f\n", i, channel_data[i]);
	// }

	// Verify the channel data with MATLAB output
	// Load the verification data from the MATLAB output file
	printf("\nLoading verification data from MATLAB output...\n");
	char ver_filepath[100];
	sprintf(ver_filepath, "%sver_chdata_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(file_name, "."), channel_num);
	// Verify the channel data with the MATLAB output
	printf("\nVerifying channel data with verification data...\n");
	verify_signals(channel_data, num_rows, import_file(ver_filepath, &ver_num_rows, &ver_num_cols), &ver_num_rows, &ver_num_cols);

	// Free the 2D array 'data'
	for (size_t i = 0; i < num_rows; i++)
	{
		free(data[i]);
	}
	free(data);

	/* ------------------------ Downsampling --------------------------- */
	// Downsample the channel data by a factor of 16
	int factor = 16;
	printf("\nDownsampling channel data by a factor of %d...\n", factor);
	float *downsampled_data = downsample(channel_data, &num_rows, factor);
	if (!downsampled_data)
	{
		printf("Error: Downsampling failed.\n");
		free(channel_data);
		free(downsampled_data);
		return 1;
	}

	// Verify the downsampled data with MATLAB downsampled output
	sprintf(ver_filepath, "%sver_ds_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(file_name, "."), channel_num);
	// Verify the channel data with the MATLAB output
	printf("\nVerifying downsampled data with MATLAB downsampled output data...\n");
	float **verify_data = import_file(ver_filepath, &ver_num_rows, &ver_num_cols);
	if (!verify_data)
	{
		printf("Error: Failed to load verification data.\n");
		free(downsampled_data);
		free(channel_data);
		return 1;
	}

	if (verify_signals(downsampled_data, num_rows, verify_data, &ver_num_rows, &ver_num_cols))
	{
		printf("Error: Downsampled data verification failed.\n");
		free(verify_data); // Free allocated memory
		free(downsampled_data);
		free(channel_data);
		return 1;
	}

	// Free verification data after use
	for (size_t i = 0; i < ver_num_rows; i++)
	{
		free(verify_data[i]);
	}
	free(verify_data);
	free(downsampled_data);
	free(channel_data);

	return 0;
}