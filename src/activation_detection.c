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
int moving_average_filtering(double *in_signal, double *out_signal, int out_signal_len, int sample_rate)
{
	if (in_signal == NULL || out_signal == NULL)
	{
		printf("\nError: Input or output signal pointer is NULL.\n");
		return 1;
	}
	int time = 1;
	int window_size = sample_rate * time; // Window size of 1 second
	if (window_size > out_signal_len)
	{
		printf("\nError: Window size is larger than signal length.\n");
		return 1;
	}

	double sum[BUFFER_SIZE + HPF_FILTER_ORDER - 1]; // Initialize sum array for moving average
	for (int i = 0; i < out_signal_len; i++)
	{
		sum[i] = 0;
		// Initialize output signal to avoid undefined behavior
		out_signal[i] = 0;
	}
	// // Compute moving average using a sliding window approach with boundary handling

	for (int i = 0; i < out_signal_len - window_size; i++)
	{
		for (int j = 1; j <= window_size; j++) // I think the j starts from 1 because the first value of input signal, which is signal from neo_transform, which has value of 1 in index 0 due to how neo_transform is implemented. This may need some refactoring in the future.
		{
			sum[i] += in_signal[i + j];
			// printf("sum[%d]%d: %f\n", i, j, sum[i]);
		}
		out_signal[i] = sum[i] / window_size;
	}

	return 0; // Success
}

int edge_detection(double *in_signal, double *edge_signal, int signal_len)
{
	double kernel[] = {-1, 0, 1}; // Edge detection kernel

	for (int i = 1; i < signal_len - 1; i++)
	{
		edge_signal[i] = (in_signal[i - 1] * kernel[0]) + (in_signal[i] * kernel[1]) + (in_signal[i + 1] * kernel[2]);
	}

	// Set first and last values to zero (to maintain array size)
	edge_signal[0] = 0;
	edge_signal[signal_len - 1] = 0;

	return 0;
}
