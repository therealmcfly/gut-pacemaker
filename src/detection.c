#include "detection.h"

#include <stdlib.h> // lib for malloc for debugging, remove after use

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
		for (int j = 1; j <= window_size; j++) // I think the j starts from 1 because the first value of input signal, which is signal from neo_transform, which has value of 1 in index 0 due to how neo_transform is implemented. Leaving is as it is now since the goal is to mirror the MATLAB code. This may need to be refactored in the future along with the neo_transform function.
		{
			sum[i] += in_signal[i + j];
			// printf("sum[%d]%d: %f\n", i, j, sum[i]);
		}
		out_signal[i] = sum[i] / window_size;
	}

	return 0; // Success
}

int edge_detection(const double *in_processed_signal, int in_processed_sig_len, const double *in_neo_signal, int in_neo_sig_len, double *out_ed_signal, int out_ed_signal_len)
{
	double kernel[] = {-1, 0, 1}; // Edge detection kernel
	int kernel_len = sizeof(kernel) / sizeof(kernel[0]);
	int half_k = kernel_len / 2; // Kernel midpoint

	double conv_signal[NEO_MAF_ED_SIGNAL_SIZE]; // Full convolution size

	conv_1d_same(in_processed_signal, in_processed_sig_len, kernel, kernel_len, conv_signal);

	double sum = 0;
	for (int i = 0; i < out_ed_signal_len; i++)
	{
		// Perform element-wise multiplication with correct indexing

		out_ed_signal[i] = conv_signal[i + 1] * in_neo_signal[i]; // Offset neo_filtered by 1
		// conv_signal is length 1 greater than in_neo_signal, so we offset by 1 to match the matlab code
		// may need to be romoved in the future when neo transform output doesnt contain 1 extra value in the front

		// Apply squaring and summing
		if (out_ed_signal[i] < 0 || out_ed_signal[i] == -0) // if negative or -0
		{
			out_ed_signal[i] = 0; // Set values to zero
		}
		else
		{
			// Square each value
			out_ed_signal[i] = out_ed_signal[i] * out_ed_signal[i];
		}

		sum += out_ed_signal[i];

		// printf("out_ed_signal[%d]: %f\n", i, out_ed_signal[i]);
	}

	double ed_sig_base = (sum / out_ed_signal_len) * ED_SCALE_VALUE; // Calculate mean and
	for (int j = 0; j < out_ed_signal_len; j++)
	{
		if (out_ed_signal[j] < ed_sig_base)
		{
			out_ed_signal[j] = 0; // Remove values below threshold
		}
	}

	return 0;
}

void conv_1d_same(const double *input, int input_size, const double *kernel, int kernel_size, double *out_conv_signal)
{
	int pad = kernel_size / 2; // Calculate padding for 'same' output

	// Perform convolution directly on the output buffer
	for (int i = 0; i < input_size; i++)
	{
		out_conv_signal[i] = 0.0; // Initialize output at index i

		for (int j = 0; j < kernel_size; j++)
		{
			int input_idx = i + j - pad; // Adjust index for centering kernel

			// **Fix: Clamp input index to valid range**
			if (input_idx < 0)
				input_idx = 0; // Mirror first element
			else if (input_idx >= input_size)
				input_idx = input_size - 1; // Mirror last element

			out_conv_signal[i] += input[input_idx] * kernel[kernel_size - 1 - j]; // Apply convolution with flipped kernel
		}
		// printf("output[%d]: %f\n", i, out_conv_signal[i]);
	}
}

/*------------------------------------------------------------------------*/
/*                            prev functions                              */
/*------------------------------------------------------------------------*/

// void conv1d_same(const double *input, int input_size,
// 								 const double *kernel, int kernel_size,
// 								 double *output)
// {
// 	int pad = kernel_size / 2; // Calculate padding for 'same' output

// 	int output_size = input_size + kernel_size - 1;								// Full convolution size
// 	double *temp = (double *)calloc(output_size, sizeof(double)); // Temporary buffer for full convolution

// 	// Initialize output array to zero
// 	for (int i = 0; i < output_size; i++)
// 	{
// 		temp[i] = 0.0;
// 	}

// 	// Perform convolution (as per MATLAB's conv2 definition)
// 	for (int i = 0; i < input_size; i++)
// 	{
// 		for (int j = 0; j < kernel_size; j++)
// 		{
// 			temp[i + j] += input[i] * kernel[j]; // No flipping of kernel
// 		}
// 	}
// 	// Extract the "same" part (centered)
// 	for (int i = 0; i < input_size; i++)
// 	{
// 		output[i] = temp[i + pad];
// 		printf("output[%d]: %f\n", i, output[i]);
// 	}
// }