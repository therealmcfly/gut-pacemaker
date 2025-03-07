#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../inc/file_io.h"
#include "../inc/pre_processing.h"
#include "../inc/result_check.h"

#define MATLAB_DIRECTORY "MATLAB Model/"
#define TARGET_FREQUENCY 32 // Signal frequency in Hz
#define DEBUG 1

int main(int argc, char *argv[])
{
	char file_name[100];		 // Buffer for file name
	int channel_num;				 // Buffer for channel number
	int data_frequency;			 // Buffer for exp data frequency
	size_t num_rows = 0;		 // Variable to store the number of rows read
	size_t num_cols = 0;		 // Variable to store the number of columns read
	size_t ver_num_rows = 0; // Variable to store the number of rows for verification data
	size_t ver_num_cols = 0; // Variable to store the number of columns for verification data

	char ver_filepath[100]; // Buffer for verification file path

	// If no arguments are passed, ask for user input
	if (argc != 3)
	{
		// Ask for filename and validate input
		get_file_name(file_name, sizeof(file_name), &data_frequency);
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
		if (validate_file_name(file_name, &data_frequency))
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

	/* ---------------------------------------------------------------------- */
	/* -------------------------- Pre Processing ---------------------------- */
	/* ---------------------------------------------------------------------- */
	// code below here is likely to be used in embedded systems. lib usage may be restricted, memory and resources may be limited

	/* ------------------------ Channel Data Retrieval --------------------------- */
	// Get the channel data
	float *channel_data = get_ch_signal(data, num_rows, num_cols, channel_num);
	// Free the 2D array 'data'
	for (size_t i = 0; i < num_rows; i++)
	{
		free(data[i]);
	}
	free(data);
	if (!channel_data)
	{
		printf("Error: Failed to retrieve channel data.\n");
		return 1;
	}

	// // DEBUG: Print the first 10 samples of the channel
	// for (size_t i = 0; i < 10 && i < num_rows; i++)
	// {
	// 	printf("channel[%zu] = %f\n", i, channel_data[i]);
	// }

	/* ---------------- Channel Retrieval Verification ----------------- */
	// // Verify the channel data with MATLAB output
	// // Load the verification data from the MATLAB output file
	// printf("\n-----------------CHANNEL VERIFICATION-----------------\n");
	// printf("\nLoading verification data from MATLAB output...\n");
	// sprintf(ver_filepath, "%sver_chdata_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(file_name, "."), channel_num);
	// // Verify the channel data with the MATLAB output
	// printf("\nVerifying channel data with verification data...\n");
	// verify_signals(channel_data, num_rows, import_file(ver_filepath, &ver_num_rows, &ver_num_cols), &ver_num_rows, &ver_num_cols);
	// printf("-----------------VERIFICATION SUCCESSFUL-----------------\n");
	/* ----------------------------------------------------------------- */

	/* ------------------------ Downsampling --------------------------- */
	// Downsample the channel data if the initial data frequency is higher than the desired frequency
	float *downsampled_data = NULL;
	if (data_frequency >= TARGET_FREQUENCY * 2)
	{
		printf("\nData frequency: %d Hz\n", data_frequency);
		printf("Desired frequency: %d Hz\n", TARGET_FREQUENCY);
		printf("Data frequency is higher than the desired frequency.\n");
		int factor = data_frequency / TARGET_FREQUENCY;
		printf("Downsampling channel data by a factor of %d...\n", factor);
		downsampled_data = downsample(channel_data, &num_rows, factor);
		if (!downsampled_data)
		{
			printf("Error: Downsampling failed.\n");
			free(channel_data);
			free(downsampled_data);
			return 1;
		}

		/* ---------------- Downsampling Verification ----------------- */
		// Verify the downsampled data with MATLAB downsampled output
		// Verify the channel data with the MATLAB output
		printf("\n-----------------DOWN-SAMPLING VERIFICATION-----------------\n");
		sprintf(ver_filepath, "%sver_ds_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(file_name, "."), channel_num);
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
		printf("-----------------VERIFICATION SUCCESSFUL-----------------\n");
		/* ----------------------------------------------------------------- */
	}
	else
	{
		printf("Data frequency: %d Hz\n", data_frequency);
		printf("Desired frequency: %d Hz\n", TARGET_FREQUENCY);
		printf("\nData frequency is lower than the desired frequency.\n");
		printf("No downsampling required.\n");
	}

	/* ------------------------ Post Processing --------------------------- */
	// Post processing steps can be added here

	// ------------------------ Free Memory ---------------------------
	// Free allocated memory

	!downsampled_data ? printf("No downsampled data to free.") : free(downsampled_data);
	!channel_data ? printf("No channel data to free.") : free(channel_data);

	return 0;
}