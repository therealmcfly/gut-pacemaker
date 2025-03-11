#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "filtering.h"

#define FILTER_ORDER 58
int lowpass_filter(double *in_signal, double *lpf_signal, int signal_length)
{
	double fir_coeffs[FILTER_ORDER + 1] = {
			-0.000175523523401278, -4.21964700295067e-05, 0.000358539018147976,
			0.000818885111422960, 0.000880009800921136, 0.000146593519844785,
			-0.00126741644966190, -0.00253045483942841, -0.00244556829945420,
			-0.000319706972940245, 0.00317857417147882, 0.00592145559962027,
			0.00539613367777395, 0.000551550182268367, -0.00676355540997375,
			-0.0120612610944701, -0.0105955267789609, -0.000810680537608409,
			0.0133368278569080, 0.0232430247578836, 0.0201144233650769,
			0.00105005485701899, -0.0268395094187461, -0.0474572229190745,
			-0.0425069593752636, -0.00121950816368866, 0.0720776679538792,
			0.157159542594032, 0.225209268764937, 0.251185078042975,
			0.225209268764937, 0.157159542594032, 0.0720776679538792,
			-0.00121950816368866, -0.0425069593752636, -0.0474572229190745,
			-0.0268395094187461, 0.00105005485701899, 0.0201144233650769,
			0.0232430247578836, 0.0133368278569080, -0.000810680537608409,
			-0.0105955267789609, -0.0120612610944701, -0.00676355540997375,
			0.000551550182268367, 0.00539613367777395, 0.00592145559962027,
			0.00317857417147882, -0.000319706972940245, -0.00244556829945420,
			-0.00253045483942841, -0.00126741644966190, 0.000146593519844785,
			0.000880009800921136, 0.000818885111422960, 0.000358539018147976,
			-4.21964700295067e-05, -0.000175523523401278};

	double padded_signal[PADDED_BUFFER_SIZE];
	for (int i = 0; i < PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		padded_signal[i] = 0.0;
	}
	double padded_lpf_signal[PADDED_BUFFER_SIZE];
	for (int i = 0; i < PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		padded_lpf_signal[i] = 0.0;
	}

	// Step 1: Apply PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	double start_signal = in_signal[0];
	double end_signal = in_signal[signal_length - 1];
	for (int i = 0; i < signal_length + (PADDING_SIZE * 2); i++)
	{
		if (i < PADDING_SIZE)
		{
			padded_signal[i] = start_signal; // First PADDING_SIZE padding to left with start_signal
		}
		else if (i >= PADDING_SIZE && i < PADDING_SIZE + signal_length)
		{
			padded_signal[i] = in_signal[i - PADDING_SIZE]; // Copying signal to middle of padded_signal
		}
		else
		{
			padded_signal[i] = end_signal; // Last PADDING_SIZE padding to right with end_signal
		}
	}

	if (fir_filter(fir_coeffs, FILTER_ORDER, padded_signal, padded_lpf_signal, signal_length + (PADDING_SIZE * 2)))
	{
		printf("Error: FIR filtering failed.\n");
		return 1;
	}

	// ✅ Step 5: Reverse Again to Restore Order
	for (int i = 0; i < signal_length; i++)
	{
		lpf_signal[i] = padded_lpf_signal[i + PADDING_SIZE];
		// printf("lpf_signal[%d]: %.15f\n", i, lpf_signal[i]);
	}

	// check the padding removal
	if (lpf_signal[0] != padded_lpf_signal[PADDING_SIZE] || lpf_signal[signal_length - 1] != padded_lpf_signal[PADDING_SIZE + signal_length - 1])
	{
		printf("\nError in lowpass_filter(): Padding removal failed, check the padding logic.\n");
		printf("lpf_signal[%d]: %.15f\n", 0, lpf_signal[0]);
		printf("lpf_signal_wp[%d]: %.15f\n", PADDING_SIZE, padded_lpf_signal[PADDING_SIZE]);
		printf("\nlpf_signal[%d]: %.15f\n", signal_length - 1, lpf_signal[signal_length - 1]);
		printf("lpf_signal_wp[%d]: %.15f\n", PADDING_SIZE + signal_length - 1, padded_lpf_signal[PADDING_SIZE + signal_length - 1]);
		return 1;
	}
	return 0;
}

int fir_filter(const double *coeffs, int filter_order, const double *in_signal, double *out_signal, int signal_length)
{
	int filt_delay = filter_order / 2; // N/2 delay compensation

	// Allocate memory for padded input signal (x + zeros)
	int z_padded_signal_length = signal_length + filt_delay;
	double *z_padded_signal = (double *)calloc(z_padded_signal_length, sizeof(double));
	if (!z_padded_signal)
	{
		printf("Memory allocation failed\n");
		return 1;
	}

	// Copy original input to padded input
	memcpy(z_padded_signal, in_signal, signal_length * sizeof(double));

	// Temporary buffer for filtered output
	double *temp_output = (double *)calloc(z_padded_signal_length, sizeof(double));
	if (!temp_output)
	{
		printf("Memory allocation failed\n");
		free(z_padded_signal);
		return 1;
	}

	// Apply FIR filter
	for (int n = 0; n < z_padded_signal_length; n++)
	{
		double yn = 0.0; // Initialize output sample

		// printf("\nProcessing out_signal[%d]:\n", n); // Debug

		for (int i = 0; i < filter_order + 1 /*<== filter order is coeff length -1 */; i++) // Apply filter
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

	// Free allocated memory
	free(z_padded_signal);
	free(temp_output);

	return 0;
}
