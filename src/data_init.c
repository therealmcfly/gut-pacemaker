#include "data_init.h"
#include <stdio.h>	// for printf (if desired)
#include <stdlib.h> // for malloc/free (if allowed in your environment)
#include <string.h> // for strtok, strncpy, etc.
#include "file_io.h"
#include "config.h"

double *get_sample_data(int user_argc, char *user_argv[], size_t *out_data_length, int *out_channel_num, char *out_file_name, int *out_cur_data_freq)
{ // Buffer for channel number
	// int data_frequency;	 // Buffer for exp data frequency
	size_t num_rows = 0; // Variable to store the number of rows read
	size_t num_cols = 0; // Variable to store the number of columns read
#ifdef DATA_VERIFICATION || CHANNEL_RETRIEVAL_VERIFICATION || DOWNSAMPING_VERIFICATION
	size_t ver_num_rows = 0; // Variable to store the number of rows for verification data
	size_t ver_num_cols = 0; // Variable to store the number of columns for verification data
	char ver_filepath[100];	 // Buffer for verification file path
#endif

	// If no arguments are passed, ask for user input
	if (user_argc != 3)
	{
		// Ask for filename and validate input
		get_file_name(out_file_name, out_cur_data_freq);
		// Ask for channel number and validate input
		get_channel_num(out_channel_num, MAX_CHANNEL);
	}
	else
	{
		// Handle command line arguments
		// Safely copy arguments to prevent buffer overflow

		int arg_len = strlen(user_argv[1]);

		strncpy(out_file_name, user_argv[1], arg_len);
		out_file_name[arg_len] = '\0';				 // Ensure null termination
		*out_channel_num = atoi(user_argv[2]); // Convert argument to integer

		// Validate file name & channel number
		if (validate_file_name(out_file_name, out_cur_data_freq))
			return NULL; // Exit with error
		if (validate_channel_num(*out_channel_num, MAX_CHANNEL))
			return NULL; // Exit with error
	}

	printf("\n");
	printf("----------------------------------------\n");
	printf("Reading from file: %s\n", out_file_name);
	printf("Channel number: %d\n", *out_channel_num);
	printf("----------------------------------------\n");

	// Read data from file
	printf("\nReading data from file...\n");
	double **data = import_file(out_file_name, &num_rows, &num_cols);
	if (!data)
	{
		printf("Error: Failed to load data.\n");
		return NULL;
	}

	/* ---------------------------------------------------------------------- */
	/* -------------------------- Pre Processing ---------------------------- */
	/* ---------------------------------------------------------------------- */
	// code below here is likely to be used in embedded systems. lib usage may be restricted, memory and resources may be limited

	// CHANNEL SIGNAL RETRIEVAL
	// Get the channel data
	double *channel_data = get_ch_signal(data, num_rows, num_cols, *out_channel_num);
	// Free the 2D array 'data'
	for (size_t i = 0; i < num_rows; i++)
	{
		free(data[i]);
	}
	free(data);
	if (!channel_data)
	{
		printf("Error: Failed to retrieve channel data.\n");
		return NULL;
	}

// // DEBUG: Print the first 10 samples of the channel
// for (size_t i = 0; i < 10 && i < num_rows; i++)
// {
// 	printf("channel[%zu] = %f\n", i, channel_data[i]);
// }

/* ---------------- Channel Retrieval Verification ----------------- */
#ifdef CHANNEL_RETRIEVAL_VERIFICATION
	// Verify the channel data with MATLAB output
	// Load the verification data from the MATLAB output file
	printf("\n-----------------CHANNEL VERIFICATION-----------------\n");
	printf("\nLoading verification data from MATLAB output...\n");
	sprintf(ver_filepath, "%sver_chdata_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(out_file_name, "."), *out_channel_num);
	// Verify the channel data with the MATLAB output
	printf("\nVerifying channel data with verification data...\n");
	double **verify_ch_data = import_file(ver_filepath, &ver_num_rows, &ver_num_cols);
	if (!verify_ch_data)
	{
		printf("Error: Failed to load verification data.\n");
		free(channel_data);
		return NULL;
	}
	// for (size_t i = 0; i < ver_num_rows; i++)
	// {
	// 	printf("verify_ch_data[%zu] = %f\n", i, verify_ch_data[i][0]);
	// }

	if (verify_signals(channel_data, num_rows, verify_ch_data, &ver_num_rows, &ver_num_cols))
	{
		printf("\nError: Channel data verification failed.\n");
		free(channel_data); // Free allocated memory
		return NULL;
	}

	// Free verification data after use
	for (size_t i = 0; i < ver_num_rows; i++)
	{
		free(verify_ch_data[i]);
	}
	free(verify_ch_data);
	printf("-----------------VERIFICATION SUCCESSFUL-----------------\n");
#endif
	/* ----------------------------------------------------------------- */

	// DOWNSAMPLING
	// Downsample the channel data if the initial data frequency is higher than the desired frequency
	double *downsampled_data = NULL;
	if (*out_cur_data_freq >= TARGET_FREQUENCY * 2)
	{
		printf("\nCurrent data frequency: %d Hz\n", *out_cur_data_freq);
		printf("Desired frequency: %d Hz\n", TARGET_FREQUENCY);
		printf("Current data frequency is higher than the desired frequency.\n");
		int factor = *out_cur_data_freq / TARGET_FREQUENCY;
		printf("Downsampling channel data by a factor of %d...\n", factor);
		downsampled_data = downsample(channel_data, &num_rows, factor);
		if (!downsampled_data)
		{
			printf("Error: Downsampling failed.\n");
			free(channel_data);
			return NULL;
		}
		*out_cur_data_freq = TARGET_FREQUENCY; // Update the data frequency
		printf("Successfully downsampled to %d Hz\n", *out_cur_data_freq);

/* ---------------- Downsampling Verification ----------------- */
#ifdef DOWNSAMPING_VERIFICATION
		// Verify the downsampled data with MATLAB downsampled output
		// Verify the channel data with the MATLAB output
		printf("\n-----------------DOWN-SAMPLING VERIFICATION-----------------\n");
		sprintf(ver_filepath, "%sver_ds_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(out_file_name, "."), *out_channel_num);
		double **verify_data = import_file(ver_filepath, &ver_num_rows, &ver_num_cols);
		if (!verify_data)
		{
			printf("Error: Failed to load verification data.\n");
			free(downsampled_data);
			free(channel_data);
			return NULL;
		}

		if (verify_signals(downsampled_data, num_rows, verify_data, &ver_num_rows, &ver_num_cols))
		{
			printf("Error: Downsampled data verification failed.\n");
			free(verify_data); // Free allocated memory
			free(downsampled_data);
			free(channel_data);
			return NULL;
		}
		// Free verification data after use
		for (size_t i = 0; i < ver_num_rows; i++)
		{
			free(verify_data[i]);
		}
		free(verify_data);
		printf("-----------------VERIFICATION SUCCESSFUL-----------------\n");
#endif

		// Free allocated memory if downsampling was successful
		if (downsampled_data && channel_data)
		{
			channel_data = NULL;
		}

		free(channel_data);

		*out_data_length = num_rows;

		return downsampled_data;
	}
	else
	{
		printf("Current data frequency: %d Hz\n", *out_cur_data_freq);
		printf("Desired frequency: %d Hz\n", TARGET_FREQUENCY);
		printf("\nCurrent data frequency is lower than the desired frequency.\n");
		printf("No downsampling required. Current frequecy is still at %d Hz\n", *out_cur_data_freq);
		*out_data_length = num_rows;

		return channel_data;
	}
}

/**
 * @brief Downsamples the input signal by the specified factor.
 *
 * The caller is responsible for freeing the returned memory
 * by calling free() when done with it.
 */
double *downsample(const double *inSignal, size_t *signal_length, int factor)
{

	if (!inSignal || !signal_length || *signal_length <= 0 || factor <= 0)
	{
		printf("Error: Invalid input.\n");
		return NULL;
	}
	size_t new_length = *signal_length / factor;
	if (new_length == 0)
	{
		printf("Error: Downsampled data length cannot be zero.\n");
		return NULL;
	}
	double *outSignal = (double *)malloc(*signal_length / factor * sizeof(double));

	if (!outSignal)
	{
		printf("Error: Memory allocation failed.\n");
		return NULL;
	}
	for (size_t i = 0; i < new_length; i++)
	{
		outSignal[i] = inSignal[i * factor]; // Downsampling
#if DEBUG
		if (i >= START_ROW && i <= END_ROW)
		{
			printf("Original signal[%zu]: %f\n", i * factor, inSignal[i * factor]);
			printf("Downsampled signal[%zu]: %f\n", i, outSignal[i]);
		}
#endif
	}

	printf("Downsampling complete: %zu samples -> %zu samples\n", *signal_length, new_length);
	*signal_length = new_length; // Update the new length
	return outSignal;
}

/**
 * @brief Retrieve all samples from a single channel (1-based).
 *
 * Returns a newly allocated array of length 'num_rows' containing
 * the requested channel's data. Returns NULL on error.
 *
 * @note The caller is responsible for freeing the returned memory
 * by calling free() when done with it.
 */
double *get_ch_signal(double **data, size_t num_rows, size_t num_cols, int channel_num)
{
	printf("\nRetrieving signal data from channel %d from data...\n", channel_num);
	// Validate inputs
	if (!data)
	{
		printf("Error (get_signal): data pointer is NULL.\n");
		return NULL;
	}

	if (channel_num < 1 || (size_t)channel_num > num_cols)
	{
		printf("Error (get_signal): Invalid channel number %d. Must be between 1 and %zu.\n",
					 channel_num, num_cols);
		return NULL;
	}

	// Allocate array for the requested channel
	double *channel_data = (double *)malloc(num_rows * sizeof(double));
	if (!channel_data)
	{
		printf("Error: Could not allocate memory for channel data.\n");
		return NULL;
	}
	// Note: 'channel_num' is 1-based, so subtract 1 for 0-based index
	size_t ch_index = (size_t)(channel_num - 1);
	for (size_t row = 0; row < num_rows; row++)
	{
		channel_data[row] = data[row][ch_index];
	}
	printf("Signal data retrieved: %zu samples\n", num_rows);

	return channel_data;
}

int verify_signals(double *signal, size_t num_of_data, double **verify_data, size_t *verify_num_rows, size_t *verify_num_cols)
{
	// compare rows and columns of data and verify_data
	if (*verify_num_rows != num_of_data)
	{
		printf("Error: Number of rows do not match.\n\n");
		printf("verify_num_rows: %zu\n", *verify_num_rows);
		printf("num_of_data: %zu\n", num_of_data);
		return 1;
	}

	printf("Row match : %zu row(s)\n", *verify_num_rows);

	if (*verify_num_cols != 1)
	{
		printf("Error: Number of columns do not match.\n\n");
		printf("verify_num_cols: %zu\n", *verify_num_cols);
		printf("data_num_cols: %d\n", 1);
		return 1;
	}

	printf("Column match : %zu column(s)\n", *verify_num_cols);

	// Print Table Header
	printf("\n---------------------------------------------------\n");
	printf("| Index |      signal      |    verify_data   |\n");
	printf("---------------------------------------------------\n");

	// compare each row of data and verify_data
	int count = 0;
	for (size_t row = 0; row < num_of_data; row++)
	{
		// Print data row with proper alignment
		if (row >= START_ROW && row <= END_ROW)
		{
			printf("| %5zu | %16.6f | %16.6f |\n", row, signal[row], verify_data[row][0]);
		}
		if (row == END_ROW + 1)
		{
			printf("---------------------------------------------------\n");
			printf("... and %zu more rows\n", num_of_data - END_ROW - 1);
			break;
		}

		// Check for mismatch
		if (signal[row] != verify_data[row][0])
		{
			printf("---------------------------------------------------\n");
			printf("Error: Data mismatch at row %zu.\n\n", row);
			printf("signal in row %zu: %f\n", row, signal[row]);
			printf("verify_data in row %zu: %f\n", row, verify_data[row][0]);
			return 1;
		}

		count++;
	}

	// Print closing line
	printf("Data match : %lld data(s) checked\n", num_of_data);
	return 0;
}