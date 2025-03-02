#include "../inc/pre_processing.h"
#include <stdio.h>	// for printf (if desired)
#include <stdlib.h> // for malloc/free (if allowed in your environment)

#define DEBUG 0
#define START_ROW 0
#define END_ROW 10
/**
 * @brief Downsamples the input signal by the specified factor.
 */
float *downsample(const float *inSignal,
									size_t *signal_length,
									int factor)
{

	if (!inSignal || signal_length <= 0 || factor <= 0)
	{
		printf("Error: Invalid input.\n");
		return NULL;
	}
	size_t new_length = *signal_length / factor;
	float *outSignal = (float *)malloc(*signal_length / factor * sizeof(float));

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
 */
float *get_ch_signal(float **data, size_t num_rows, size_t num_cols, int channel_num)
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
	float *channel_data = (float *)malloc(num_rows * sizeof(float));
	if (!channel_data)
	{
		printf("Error: Could not allocate memory for channel data.\n");
		return NULL;
	}

	// Copy the requested channel's values into 'channel_data'
	// Note: 'channel_num' is 1-based, so subtract 1 for 0-based index
	size_t ch_index = (size_t)(channel_num - 1);
	for (size_t row = 0; row < num_rows; row++)
	{
		channel_data[row] = data[row][ch_index];
	}
	printf("Signal data retrieved: %zu samples\n", num_rows);

	return channel_data;
}
