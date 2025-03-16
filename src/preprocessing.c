#include "preprocessing.h"

int lowpass_filter(double *in_signal, double *lpf_signal, int signal_length)
{
	// Low-pass filter constants
#define LPF_PADDING_SIZE 60
#define LPF_PADDED_BUFFER_SIZE ((BUFFER_SIZE + 1 /* + 1 to mirror the MATLAB logic, romove if not needed*/) + (2 * LPF_PADDING_SIZE))
	const double COEFFS[] = {
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
	int filter_order = sizeof(COEFFS) / sizeof(COEFFS[0]) - 1;

	double padded_signal[LPF_PADDED_BUFFER_SIZE];
	for (int i = 0; i < LPF_PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		padded_signal[i] = 0.0;
	}
	double padded_lpf_signal[LPF_PADDED_BUFFER_SIZE];
	for (int i = 0; i < LPF_PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		padded_lpf_signal[i] = 0.0;
	}

	// Step 1: Apply PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	apply_padding(in_signal, signal_length, padded_signal, LPF_PADDED_BUFFER_SIZE, LPF_PADDING_SIZE);

	// Step 2: Apply FIR Filtering
	if (lowpass_fir_filter(COEFFS, filter_order + 1, padded_signal, padded_lpf_signal, signal_length + (LPF_PADDING_SIZE * 2)))
	{
		printf("Error: FIR filtering failed.\n");
		return 1;
	}

	// Step 3: Remove padding
	for (int i = 0; i < signal_length; i++)
	{
		lpf_signal[i] = padded_lpf_signal[i + LPF_PADDING_SIZE];
		// printf("lpf_signal[%d]: %.15f\n", i, lpf_signal[i]);
	}

	// check the padding removal
	if (lpf_signal[0] != padded_lpf_signal[LPF_PADDING_SIZE] || lpf_signal[signal_length - 1] != padded_lpf_signal[LPF_PADDING_SIZE + signal_length - 1])
	{
		printf("\nError in lowpass_filter(): Padding removal failed, check the padding logic.\n");
		printf("lpf_signal[%d]: %.15f\n", 0, lpf_signal[0]);
		printf("lpf_signal_wp[%d]: %.15f\n", LPF_PADDING_SIZE, padded_lpf_signal[LPF_PADDING_SIZE]);
		printf("\nlpf_signal[%d]: %.15f\n", signal_length - 1, lpf_signal[signal_length - 1]);
		printf("lpf_signal_wp[%d]: %.15f\n", LPF_PADDING_SIZE + signal_length - 1, padded_lpf_signal[LPF_PADDING_SIZE + signal_length - 1]);
		return 1;
	}

	return 0;
}

int lowpass_fir_filter(const double *coeffs, int coeff_len, const double *in_signal, double *out_signal, int signal_length)
{
	int filt_delay = (coeff_len - 1) / 2; // filter order is coeff length -1, and delay is half of the filter order

	// Allocate memory for padded input signal (x + zeros)
	int z_padded_signal_length = signal_length + filt_delay;
	double *z_padded_signal = (double *)calloc(z_padded_signal_length, sizeof(double));
	if (!z_padded_signal)
	{
		printf("Memory allocation failed\n");
		return 1;
	}

	// Copy original input to padded input
	for (int i = 0; i < signal_length; i++)
	{
		z_padded_signal[i] = in_signal[i];
	}

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

	// Free allocated memory
	free(z_padded_signal);
	free(temp_output);

	return 0;
}

int highpass_filter(double *in_signal, double *hpf_signal, int signal_len)
{
	// High-pass filter constants
#define HPF_PADDING_SIZE 60
#define HPF_PADDED_BUFFER_SIZE ((BUFFER_SIZE + 1 /* + 1 to mirror the MATLAB logic, romove if not needed*/) + (2 * HPF_PADDING_SIZE))
	const double COEFFS[] = {
			-0.00788739916497821, -0.00883748945388316, -0.00982539957188940, -0.0108458743104990,
			-0.0118931431763899, -0.0129609677159769, -0.0140426954726160, -0.0151313200039590,
			-0.0162195462972988, -0.0172998608393711, -0.0183646055252676, -0.0194060545299731,
			-0.0204164932165573, -0.0213882981180202, -0.0223140170058552, -0.0231864480479760,
			-0.0239987170620023, -0.0247443518870506, -0.0254173529279569, -0.0260122589698998,
			-0.0265242074181275, -0.0269489881861568, -0.0272830905354722, -0.0275237422592968,
			-0.0276689407011917, 0.972282524795331, -0.0276689407011917, -0.0275237422592968,
			-0.0272830905354722, -0.0269489881861568, -0.0265242074181275, -0.0260122589698998,
			-0.0254173529279569, -0.0247443518870505, -0.0239987170620022, -0.0231864480479760,
			-0.0223140170058552, -0.0213882981180202, -0.0204164932165573, -0.0194060545299731,
			-0.0183646055252676, -0.0172998608393711, -0.0162195462972988, -0.0151313200039590,
			-0.0140426954726160, -0.0129609677159769, -0.0118931431763899, -0.0108458743104990,
			-0.00982539957188940, -0.00883748945388316, -0.00788739916497821};

	int filter_order = sizeof(COEFFS) / sizeof(COEFFS[0]) - 1;

	double padded_signal[HPF_PADDED_BUFFER_SIZE];
	for (int i = 0; i < HPF_PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		padded_signal[i] = 0.0;
	}
	double padded_hpf_signal[HPF_PADDED_BUFFER_SIZE];
	for (int i = 0; i < HPF_PADDED_BUFFER_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		padded_hpf_signal[i] = 0.0;
	}
	// Step 1: Apply HPF_PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	apply_padding(in_signal, signal_len, padded_signal, HPF_PADDED_BUFFER_SIZE, HPF_PADDING_SIZE);

	// Step 2: Apply FIR Filtering
	if (highpass_fir_filter(COEFFS, filter_order + 1, padded_signal, padded_hpf_signal, signal_len + (HPF_PADDING_SIZE * 2)))
	{
		printf("Error: FIR filtering failed.\n");
		return 1;
	}

	// Step 3: Remove padding
	for (int i = 0; i < signal_len; i++)
	{
		hpf_signal[i] = padded_hpf_signal[i + HPF_PADDING_SIZE];
		// printf("lpf_signal[%d]: %.15f\n", i, lpf_signal[i]);
	}

	// check the padding removal
	if (hpf_signal[0] != padded_hpf_signal[HPF_PADDING_SIZE] || hpf_signal[signal_len - 1] != padded_hpf_signal[HPF_PADDING_SIZE + signal_len - 1])
	{
		printf("\nError in highpass_filter(): Padding removal failed, check the padding logic.\n");
		printf("hpf_signal[%d]: %.15f\n", 0, hpf_signal[0]);
		printf("hpf_signal_wp[%d]: %.15f\n", HPF_PADDING_SIZE, padded_hpf_signal[HPF_PADDING_SIZE]);
		printf("\nhpf_signal[%d]: %.15f\n", signal_len - 1, hpf_signal[signal_len - 1]);
		printf("hpf_signal_wp[%d]: %.15f\n", HPF_PADDING_SIZE + signal_len - 1, padded_hpf_signal[HPF_PADDING_SIZE + signal_len - 1]);
		return 1;
	}
	return 0;
}

int highpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, double *out_signal, int signal_length)
{
	// Unlike the lowpass_fir_filter, the highpass_fir_filter doesn't account for filter delay
	// Perform Convolution
	for (int i = 0; i < signal_length; i++)
	{
		out_signal[i] = 0.0; // Initialize
		for (int j = 0; j < num_coeffs; j++)
		{
			if (i - j >= 0 && i - j < signal_length)
			{
				out_signal[i] += in_signal[i - j] * coeffs[j];
			}
		}
	}

	return 0;
}

void apply_padding(double *in_signal, int in_signal_len, double *padded_signal, int padded_signal_len, int padding_size)
{
	// Ensure the padded_signal buffer is large enough
	if (padded_signal_len < in_signal_len + (2 * padding_size))
	{
		printf("Error: padded_signal buffer too small in apply_padding\n");
		return;
	}

	// Step 1: Apply PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	double start_signal = in_signal[0];
	double end_signal = in_signal[in_signal_len - 1];
	for (int i = 0; i < in_signal_len + (padding_size * 2); i++)
	{
		if (i < padding_size)
		{
			padded_signal[i] = start_signal; // First PADDING_SIZE padding to left with start_signal
		}
		else if (i >= padding_size && i < padding_size + in_signal_len)
		{
			padded_signal[i] = in_signal[i - padding_size]; // Copying signal to middle of padded_signal
		}
		else
		{
			padded_signal[i] = end_signal; // Last PADDING_SIZE padding to right with end_signal
		}
	}
}

int detect_artifact(const double *in_signal, double *out_signal, int signal_length)
{
	return 0;
}