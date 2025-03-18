#include "activation_detection.h"

int neo_transform(double *in_signal, int in_signal_len, double *out_signal, int out_signal_len)
{
	if (in_signal_len < 3)
	{
		printf("\nError: Signal length too short for NEO Transform.\n");
		printf("Signal length: %d\n", in_signal_len);
		return 1;
	}
	if (in_signal_len - out_signal_len != 1)
	{
		printf("\nError: Output signal length must be 1 less than input signal length.\n");
		printf("Input signal length: %d\n", in_signal_len);
		printf("Output signal length: %d\n", out_signal_len);
		return 1;
	}

	// Set first values to zero (to maintain array size)
	out_signal[0] = 1;

	// Apply NEO Transform to the signal
	for (int i = 1; i < in_signal_len - 1; i++)
	{
		out_signal[i] = (in_signal[i] * in_signal[i]) - (in_signal[i - 1] * in_signal[i + 1]);
	}

	return 0;
}

// Function to apply a moving average filter with a window of 1 second
// Returns 0 on success, -1 on failure
int moving_average_filtering(double *input_signal, double *output_signal, int length, int sample_rate)
{
	if (input_signal == NULL || output_signal == NULL)
	{
		printf("\nError: Input or output signal pointer is NULL.\n");
		return 1;
	}
	int time = 1;
	int window_size = sample_rate * time; // Window size of 1 second
	if (window_size > length)
	{
		printf("\nError: Window size is larger than signal length.\n");
		return 1;
	}

	double sum[BUFFER_SIZE + HPF_FILTER_ORDER - 1]; // Initialize sum array for moving average
	for (int i = 0; i < length; i++)
	{
		sum[i] = 0;
		// Initialize output signal to avoid undefined behavior
		output_signal[i] = 0;
	}
	// // Compute moving average using a sliding window approach with boundary handling

	for (int i = 0; i < length - window_size; i++)
	{
		for (int j = 1; j <= window_size; j++) // I think the j starts from 1 because the first value of input signal, which is signal from neo_transform, which has value of 1 in index 0 due to how neo_transform is implemented. This may need some refactoring in the future.
		{
			sum[i] += input_signal[i + j];
			// printf("sum[%d]%d: %f\n", i, j, sum[i]);
		}
		output_signal[i] = sum[i] / window_size;
	}

	// for (int i = 0; i < length; i++)
	// {
	// 	// Handle boundary conditions with a centered sliding window
	// 	int start = i - window_size / 2;
	// 	if (start < 0)
	// 		start = 0;

	// 	int end = i + window_size / 2;
	// 	if (end >= length)
	// 		end = length - 1;

	// 	int actual_window_size = end - start + 1;

	// 	sum[i] = 0;
	// 	for (int j = start; j <= end; j++)
	// 	{
	// 		sum[i] += input_signal[j];
	// 		// printf("sum[%d]%d: %f\n", i, j, sum[i]);
	// 	}
	// 	output_signal[i] = sum[i] / actual_window_size;
	// }

	return 0; // Success
}
