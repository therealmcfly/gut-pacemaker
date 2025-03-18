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

	// Set first and last values to zero (to maintain array size)
	out_signal[0] = 1;

	// Apply NEO Transform to the signal
	for (int i = 1; i < in_signal_len - 1; i++)
	{
		out_signal[i] = (in_signal[i] * in_signal[i]) - (in_signal[i - 1] * in_signal[i + 1]);
	}

	return 0;
}

int moving_average_filter(double *input_signal, double *output_signal, int in_signal_len, int window_size)
{
	if (window_size <= 0)
	{
		printf("\nError: Invalid window size.\n");
		printf("Window size: %d\n", window_size);
		return 1;
	}

	for (int i = 0; i < in_signal_len; i++)
	{
		double sum = 0.0;
		int count = 0;

		// Apply moving average
		for (int j = i - window_size / 2; j <= i + window_size / 2; j++)
		{
			if (j >= 0 && j < in_signal_len)
			{
				sum += input_signal[j];
				count++;
			}
		}

		output_signal[i] = sum / count;
	}

	return 0;
}