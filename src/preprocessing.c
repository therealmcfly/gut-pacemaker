#include "preprocessing.h"
#include <stdlib.h> // for malloc/free (if allowed in your environment)

#include <stdio.h> // for printf (if desired)
#include "config.h"
#include "filter_coeffs.h" // for filter coefficients
#include "global.h"
#include "ring_buffer.h" // for RingBuffer functions

static double pre_lpf_pad_buffer[LPF_PADDED_BUFFER_SIZE];
static double post_lpf_pad_buffer[LPF_PADDED_BUFFER_SIZE];
static double hpf_pad_sig_buffer[HPF_PADDED_SIGNAL_SIZE];
static double hpf_conv_sig_buffer[HPF_CONV_PADDED_SIGNAL_SIZE];

/* ---------↓↓↓↓↓↓------- Utility Functions ---------↓↓↓↓↓↓------- */

static int apply_padding(double *in_signal, int in_signal_len, double *out_padded_signal, int out_padded_signal_len, int padding_size)
{
	// Ensure the hpf_pad_sig_buffer buffer is large enough
	if (out_padded_signal_len != in_signal_len + (2 * padding_size))
	{
		printf("\nError: out_padded_signal_len buffer size in apply_padding is incorrect. out_padded_signal_len is %d\n", out_padded_signal_len);
		return 1;
	}

	// Step 1: Apply PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	double start_signal = in_signal[0];
	double end_signal = in_signal[in_signal_len - 1];
	for (int i = 0; i < out_padded_signal_len; i++)
	{
		if (i < padding_size)
		{
			out_padded_signal[i] = start_signal; // First PADDING_SIZE padding to left with start_signal
		}
		else if (i >= padding_size && i < padding_size + in_signal_len)
		{
			out_padded_signal[i] = in_signal[i - padding_size]; // Copying signal to middle of hpf_pad_sig_buffer
		}
		else
		{
			out_padded_signal[i] = end_signal; // Last PADDING_SIZE padding to right with end_signal
		}
	}

	return 0;
}

static int apply_padding_from_rb(int in_signal_len, double *out_padded_signal, int out_padded_signal_len, int padding_size)
{
	// Ensure the hpf_pad_sig_buffer buffer is large enough
	if (out_padded_signal_len != in_signal_len + (2 * padding_size))
	{
		printf("\nError: out_padded_signal_len buffer size in apply_padding is incorrect. out_padded_signal_len is %d\n", out_padded_signal_len);
		return 1;
	}
	// Validate shared data
	if (g_shared_data.buffer == NULL)
	{
		printf("\nError: shared_data.buffer is NULL\n");
		return 1;
	}

	// Step 1: Apply PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)

	double *out_sig_mid_start_ptr = out_padded_signal + padding_size;					 // Pointer to the start of the middle part of the padded signal
	rb_snapshot(g_shared_data.buffer, out_sig_mid_start_ptr, g_buffer_offset); // Copy the ring buffer to the output padded signal

	double *start_signal = out_sig_mid_start_ptr;
	double *end_signal = out_sig_mid_start_ptr + in_signal_len - 1;

	for (int i = 0; i < padding_size; i++)
	{
		out_padded_signal[i] = *start_signal;															// First PADDING_SIZE padding to left
		out_padded_signal[(out_padded_signal_len - 1) - i] = *end_signal; // Last PADDING_SIZE padding to right
	}

	return 0;
}

// Function to compute cubic spline coefficients for a single segment
static void compute_single_spline(double x1, double y1, double dy1,
																	double x2, double y2, double dy2,
																	double *a, double *b, double *c, double *d)
{
	double h = x2 - x1;

	// Ensure h is not too small to avoid division issues
	// if (h < 1e-6) /*<--- OLD VER */
	if (h < PRECISION)
	{
		*a = y1;
		*b = dy1;
		*c = 0;
		*d = 0;
		return;
	}

	*a = y1;
	*b = dy1;
	*c = (3 * (y2 - y1) / (h * h)) - (2 * dy1 / h) - (dy2 / h);
	*d = (2 * (y1 - y2) / (h * h * h)) + ((dy1 + dy2) / (h * h));

	// // Debugging output to verify coefficients
	// printf("Spline Coefficients (x1=%.2f, x2=%.2f):\n", x1, x2);
	// printf("a: %f, b: %f, c: %f, d: %f\n", *a, *b, *c, *d);
}

// Function to evaluate the cubic spline at a given x_query
static double evaluate_spline(double a, double b, double c, double d, double x1, double x2, double x_query)
{
	double dx = x_query - x1;
	double h = x2 - x1;

	// Ensure dx does not exceed h to avoid instability
	// if (dx > h)
	// 	dx = h;
	// if (dx < 0)
	// 	dx = 0;
	if (dx < -PRECISION)
		dx = 0;
	if (dx > h + PRECISION)
		dx = h;

	return a + b * dx + c * dx * dx + d * dx * dx * dx;
}

static int lowpass_fir_filter(const double *coeffs, int coeff_len, const double *in_signal, double *out_signal, int signal_length, int is_bad_signal)
{
	int filt_delay = (coeff_len - 1) / 2; // filter order is coeff length -1, and delay is half of the filter order
	// signal_length here = SIGNAL_PROCESSING_BUFFER_SIZE + LPF_PADDING_SIZE * 2
	// filt_delay here = BAD_SIG_LPF_COEFFS_LEN or GOOD_SIG_LPF_COEFFS_LEN - 1 / 2
	int temp_output_len = (SIGNAL_PROCESSING_BUFFER_SIZE + (LPF_PADDING_SIZE * 2)) + ((is_bad_signal ? BAD_SIG_LPF_COEFFS_LEN : GOOD_SIG_LPF_COEFFS_LEN) - 1 / 2);
	double temp_output[(SIGNAL_PROCESSING_BUFFER_SIZE + (LPF_PADDING_SIZE * 2)) + ((is_bad_signal ? BAD_SIG_LPF_COEFFS_LEN : GOOD_SIG_LPF_COEFFS_LEN) - 1 / 2)];

	// Apply FIR filter
	for (int n = 0; n < temp_output_len; n++)
	{
		double yn = 0.0; // Initialize output sample

		// printf("\nProcessing out_signal[%d]:\n", n); // Debug

		for (int i = 0; i < coeff_len /*<== filter order is coeff length -1 */; i++) // Apply filter
		{
			if (n - i >= 0) // Ensure index is within bounds
			{
				// if (n > 1059)
				// printf("  Adding coeffs[%d] * in_signal[%d] = %lf * %lf\n", i, n - i, coeffs[i], in_signal[n - i]);
				yn += coeffs[i] * in_signal[n - i];
				// if (n > 1059)
				// printf("  -> yn = %lf\n", yn); // Debug
			}
		}
		temp_output[n] = yn; // Store filtered output
	}

	// Remove initial delay compensation (shift output)
	for (int i = 0; i < signal_length; i++)
	{
		out_signal[i] = temp_output[i + filt_delay]; // Compensate for delay
	}

	return 0;
}

static int highpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, int in_signal_len, double *out_conv_signal, int out_conv_signal_len)
{
	// Unlike the lowpass_fir_filter, the highpass_fir_filter doesn't account for filter delay
	// Perform Convolution
	for (int i = 0; i < out_conv_signal_len; i++)
	{
		out_conv_signal[i] = 0.0; // Initialize
		for (int j = 0; j < num_coeffs; j++)
		{
			if (i - j >= 0 && i - j < in_signal_len)
			{
				out_conv_signal[i] += in_signal[i - j] * coeffs[j];
			}
		}
	}
	return 0;
}

// Function to detect artifacts in a signal window
static int detect_artifact(const double *window, int window_size, double threshold)
{
	int peak_index = -1;
	double max_value = 0.0;

	for (int i = 0; i < window_size; i++)
	{
		double abs_val = (window[i] < 0) ? -window[i] : window[i]; // Absolute value of signal sample
		if (abs_val > threshold && abs_val > max_value)
		{
			max_value = abs_val;
			peak_index = i;
		}
	}
	return peak_index; // Returns the index of the artifact if found, else -1
}

// Function to remove an artifact using cubic spline interpolation
static void remove_artifact(double *window, int window_len, int x1, int x2)
{
	// if (x1 >= x2 || x1 < 1 || x2 >= window_len - 1) /*<--- OLD VER */
	if (x1 >= x2 || x1 < 0 || x2 >= window_len - 1)
	{
		printf("Error: Invalid x1 (%d) or x2 (%d) values\n", x1, x2);
		return;
	}

	// Compute start and end slopes
	double dy1 = window[x1] - window[x1 - 1];
	double dy2 = window[x2] - window[x2 - 1];

	// Compute spline coefficients
	double a, b, c, d;
	compute_single_spline(x1, window[x1], dy1, x2, window[x2], dy2, &a, &b, &c, &d);

	// Replace artifact region with cubic spline interpolation
	for (int i = x1; i <= x2; i++)
	{
		window[i] = evaluate_spline(a, b, c, d, x1, x2, i);
		// printf("window[%d]: %lf\n", i, window[i]); // Debugging output
	}
}

int lowpass_filter(double *lpf_signal, int signal_length, int is_bad_signal, void (*callback_unlock_mutex)(void))
{
	const double *lpf_coeff = is_bad_signal ? bad_sig_lpf_coeffs : good_sig_lpf_coeffs;
	// const double *lpf_coeff = good_sig_lpf_coeffs;

	int filter_order = is_bad_signal ? BAD_SIG_LPF_COEFFS_LEN - 1 : GOOD_SIG_LPF_COEFFS_LEN - 1;
	// int filter_order = good_sig_lpf_coeffs_len - 1;

	for (int i = 0; i < LPF_PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		pre_lpf_pad_buffer[i] = 0.0;
	}
	for (int i = 0; i < LPF_PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		post_lpf_pad_buffer[i] = 0.0;
	}

	// Step 1: Apply PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	if (apply_padding_from_rb(signal_length, pre_lpf_pad_buffer, LPF_PADDED_BUFFER_SIZE, LPF_PADDING_SIZE))
	{
		printf("Error: Padding for low pass filtering failed.\n");
		return 1;
	}

	if (callback_unlock_mutex != NULL)
	{
		callback_unlock_mutex(); // unlock mutex after padding
	}

	// Step 2: Apply FIR Filtering
	// if (lowpass_fir_filter(lpf_coeff, filter_order + 1, hpf_pad_sig_buffer, post_lpf_pad_buffer, signal_length + (LPF_PADDING_SIZE * 2)))
	if (lowpass_fir_filter(lpf_coeff, filter_order + 1, pre_lpf_pad_buffer, post_lpf_pad_buffer, signal_length + (LPF_PADDING_SIZE * 2), is_bad_signal))
	{
		printf("Error: FIR filtering failed.\n");
		return 1;
	}

	// Step 3: Remove padding
	for (int i = 0; i < signal_length; i++)
	{
		lpf_signal[i] = post_lpf_pad_buffer[i + LPF_PADDING_SIZE];
		// printf("lpf_signal[%d]: %.15f\n", i, lpf_signal[i]);
	}

	// check the padding removal
	if (lpf_signal[0] != post_lpf_pad_buffer[LPF_PADDING_SIZE] || lpf_signal[signal_length - 1] != post_lpf_pad_buffer[LPF_PADDING_SIZE + signal_length - 1])
	{
		printf("\nError in lowpass_filter(): Padding removal failed, check the padding logic.\n");
		printf("lpf_signal[%d]: %.15f\n", 0, lpf_signal[0]);
		printf("lpf_signal_wp[%d]: %.15f\n", LPF_PADDING_SIZE, post_lpf_pad_buffer[LPF_PADDING_SIZE]);
		printf("\nlpf_signal[%d]: %.15f\n", signal_length - 1, lpf_signal[signal_length - 1]);
		printf("lpf_signal_wp[%d]: %.15f\n", LPF_PADDING_SIZE + signal_length - 1, post_lpf_pad_buffer[LPF_PADDING_SIZE + signal_length - 1]);
		return 1;
	}

	return 0;
}

int highpass_filter(double *in_signal, int in_signal_len, double *out_hpf_signal, int *out_hpf_signal_len)
{
	if (hpf_coeffs_len != HPF_FILTER_ORDER + 1)
	{
		printf("\nError: Highpass filter order mismatch. HPF_FILTER_ORDER must equal coeff length -1. Check HPF_FILTER_ORDER value at config.h.\n");
		return 1;
	}

	int filter_order = hpf_coeffs_len - 1;

	for (int i = 0; i < HPF_PADDED_SIGNAL_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		hpf_pad_sig_buffer[i] = 0.0;
	}
	for (int i = 0; i < HPF_CONV_PADDED_SIGNAL_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		hpf_conv_sig_buffer[i] = 0.0;
	}
	// Step 1: Apply HPF_PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	if (apply_padding(in_signal, in_signal_len, hpf_pad_sig_buffer, HPF_PADDED_SIGNAL_SIZE, HPF_PADDING_SIZE))
	{
		printf("\nError: Padding for high pass filtering in failed.\n");
		return 1;
	}

	// Step 2: Apply FIR Filtering
	if (highpass_fir_filter(hpf_coeffs, filter_order + 1, hpf_pad_sig_buffer, HPF_PADDED_SIGNAL_SIZE, hpf_conv_sig_buffer, HPF_CONV_PADDED_SIGNAL_SIZE))
	{
		printf("\nError: FIR filtering failed.\n");
		return 1;
	}

	// Step 3: Remove padding
	for (int i = 0; i < *out_hpf_signal_len; i++)
	{
		out_hpf_signal[i] = hpf_conv_sig_buffer[i + HPF_PADDING_SIZE];
		// printf("lpf_signal[%d]: %.15f\n", i, lpf_signal[i]);
	}

	// check the padding removal
	if (out_hpf_signal[0] != hpf_conv_sig_buffer[HPF_PADDING_SIZE] || out_hpf_signal[*out_hpf_signal_len - 1] != hpf_conv_sig_buffer[HPF_PADDING_SIZE + *out_hpf_signal_len - 1])
	{
		printf("\nError in highpass_filter(): Padding removal failed, check the padding logic.\n");
		printf("hpf_signal[%d]: %.15f\n", 0, out_hpf_signal[0]);
		printf("hpf_signal_wp[%d]: %.15f\n", HPF_PADDING_SIZE, hpf_conv_sig_buffer[HPF_PADDING_SIZE]);
		printf("\nhpf_signal[%d]: %.15f\n", *out_hpf_signal_len - 1, out_hpf_signal[*out_hpf_signal_len - 1]);
		printf("hpf_signal_wp[%d]: %.15f\n", HPF_PADDING_SIZE + *out_hpf_signal_len - 1, hpf_conv_sig_buffer[HPF_PADDING_SIZE + *out_hpf_signal_len - 1]);
		return 1;
	}
	return 0;
}

int detect_remove_artifacts(double *in_signal, int signal_length)
{
	int i = 0;
	int j = i + ARTIFACT_DETECT_WINDOW_WIDTH;

	while (j < signal_length)
	{
		double window[ARTIFACT_DETECT_WINDOW_WIDTH];

		// Copy signal into temporary window
		for (int k = 0; k < ARTIFACT_DETECT_WINDOW_WIDTH; k++)
		{
			window[k] = in_signal[i + k];
		}

		// Detect artifact
		int loc = detect_artifact(window, ARTIFACT_DETECT_WINDOW_WIDTH, ARTIFACT_DETECT_THRESHOLD);
		if (loc != -1)
		{
			// Artifact found
			int x1 = loc - (ARTIFACT_SIZE / 2); //
			int x2 = loc + (ARTIFACT_SIZE / 2); //
#if DEBUG
			printf("\nArtifact detected at i = %d, loc = %d, value = %lf\n", i, loc, window[loc]);
			printf("x1 = %d, x2 = %d\n", x1, x2);
#endif

			if (x2 < ARTIFACT_DETECT_WINDOW_WIDTH && x1 >= 0) // this make sence but it is not the same as MATLAB logic
			// if (x2 < ARTIFACT_DETECT_WINDOW_WIDTH && x1 >= 2) // this mimics the MATLAB logic
			{
				printf("Removing artifact at index %d\n", i + loc);

				remove_artifact(window, ARTIFACT_DETECT_WINDOW_WIDTH, x1, x2);

				// Copy modified window back to signal
				for (int k = 0; k < ARTIFACT_DETECT_WINDOW_WIDTH; k++)
				{
					in_signal[i + k] = window[k];
				}
			}
			else
			{
#if DEBUG
				printf("Artifact is too close to the edge, skipping removal\n");
#endif
			}
			// for (int k = 0; k < ARTIFACT_DETECT_WINDOW_WIDTH; k++)
			// {
			// 	printf("out_signal[%d]: %lf\n", i + k, out_signal[i + k]);
			// }
		}

		// Move window forward
		i += ARTIFACT_DETECT_WINDOW_WIDTH / 2;
		j = i + ARTIFACT_DETECT_WINDOW_WIDTH;
	}
	return 0; // Success
}
