#include "preprocessing.h"

int lowpass_filter(double *in_signal, double *lpf_signal, int signal_length)
{
	// const double COEFFS[] = {// bad signal coeffs
	// 												 -0.000175523523401278, -4.21964700295067e-05, 0.000358539018147976,
	// 												 0.000818885111422960, 0.000880009800921136, 0.000146593519844785,
	// 												 -0.00126741644966190, -0.00253045483942841, -0.00244556829945420,
	// 												 -0.000319706972940245, 0.00317857417147882, 0.00592145559962027,
	// 												 0.00539613367777395, 0.000551550182268367, -0.00676355540997375,
	// 												 -0.0120612610944701, -0.0105955267789609, -0.000810680537608409,
	// 												 0.0133368278569080, 0.0232430247578836, 0.0201144233650769,
	// 												 0.00105005485701899, -0.0268395094187461, -0.0474572229190745,
	// 												 -0.0425069593752636, -0.00121950816368866, 0.0720776679538792,
	// 												 0.157159542594032, 0.225209268764937, 0.251185078042975,
	// 												 0.225209268764937, 0.157159542594032, 0.0720776679538792,
	// 												 -0.00121950816368866, -0.0425069593752636, -0.0474572229190745,
	// 												 -0.0268395094187461, 0.00105005485701899, 0.0201144233650769,
	// 												 0.0232430247578836, 0.0133368278569080, -0.000810680537608409,
	// 												 -0.0105955267789609, -0.0120612610944701, -0.00676355540997375,
	// 												 0.000551550182268367, 0.00539613367777395, 0.00592145559962027,
	// 												 0.00317857417147882, -0.000319706972940245, -0.00244556829945420,
	// 												 -0.00253045483942841, -0.00126741644966190, 0.000146593519844785,
	// 												 0.000880009800921136, 0.000818885111422960, 0.000358539018147976,
	// 												 -4.21964700295067e-05, -0.000175523523401278};

	const double COEFFS[] = {// good signal coeffs
													 0.000194720086237888, 0.000144930119587125, -0.000171837033034261, -0.000628819090270635, -0.000827573615659776, -0.000356935142036995, 0.000778687791273994, 0.00191555010349220, 0.00200911579138445, 0.000417689179478792, -0.00231422386294391, -0.00439206856301906, -0.00378608506916193, 0.000133188407427823, 0.00550817643781035, 0.00857801369674978, 0.00604150270935343, -0.00212401169156005, -0.0115088652086568, -0.0152356778642469, -0.00848338661742213, 0.00713185040005169, 0.0226819358786512, 0.0262354806038224, 0.0106961936678084, -0.0194009704345960, -0.0470528888039203, -0.0498720349755645, -0.0122454797441033, 0.0640048867098359, 0.157091856763482, 0.233409595561307, 0.262854967616885, 0.233409595561307, 0.157091856763482, 0.0640048867098359, -0.0122454797441033, -0.0498720349755645, -0.0470528888039203, -0.0194009704345960, 0.0106961936678084, 0.0262354806038224, 0.0226819358786512, 0.00713185040005169, -0.00848338661742213, -0.0152356778642469, -0.0115088652086568, -0.00212401169156005, 0.00604150270935343, 0.00857801369674978, 0.00550817643781035, 0.000133188407427823, -0.00378608506916193, -0.00439206856301906, -0.00231422386294391, 0.000417689179478792, 0.00200911579138445, 0.00191555010349220, 0.000778687791273994, -0.000356935142036995, -0.000827573615659776, -0.000628819090270635, -0.000171837033034261, 0.000144930119587125, 0.000194720086237888};

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
	if (apply_padding(in_signal, signal_length, padded_signal, LPF_PADDED_BUFFER_SIZE, LPF_PADDING_SIZE))
	{
		printf("Error: Padding for low pass filtering failed.\n");
		return 1;
	}

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

int highpass_filter(double *in_signal, int in_signal_len, double *out_hpf_signal, int *out_hpf_signal_len)
{
	const double COEFFS[HPF_FILTER_ORDER + 1] = {
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
			-0.00982539957188940, -0.00883748945388316, -0.00788739916497821}; // the filter order is declared in config.h. If the coeff length changes, the filter order should be updated in config.h. Else, the code will throw error below.

	if (sizeof(COEFFS) / sizeof(COEFFS[0]) != HPF_FILTER_ORDER + 1)
	{
		printf("\nError: Highpass filter order mismatch. HPF_FILTER_ORDER must equal coeff length -1. Check HPF_FILTER_ORDER value at config.h.\n");
		return 1;
	}

	int filter_order = sizeof(COEFFS) / sizeof(COEFFS[0]) - 1;

	double padded_signal[HPF_PADDED_SIGNAL_SIZE];
	for (int i = 0; i < HPF_PADDED_SIGNAL_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		padded_signal[i] = 0.0;
	}
	double hpf_conv_signal[HPF_CONV_PADDED_SIGNAL_SIZE];
	for (int i = 0; i < HPF_CONV_PADDED_SIGNAL_SIZE; i++) // this for loop is to mirror the MATLAB logic, romove if not needed
	{
		hpf_conv_signal[i] = 0.0;
	}
	// Step 1: Apply HPF_PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	if (apply_padding(in_signal, in_signal_len, padded_signal, HPF_PADDED_SIGNAL_SIZE, HPF_PADDING_SIZE))
	{
		printf("\nError: Padding for high pass filtering in failed.\n");
		return 1;
	}

	// Step 2: Apply FIR Filtering
	if (highpass_fir_filter(COEFFS, filter_order + 1, padded_signal, HPF_PADDED_SIGNAL_SIZE, hpf_conv_signal, HPF_CONV_PADDED_SIGNAL_SIZE))
	{
		printf("\nError: FIR filtering failed.\n");
		return 1;
	}

	// Step 3: Remove padding
	for (int i = 0; i < *out_hpf_signal_len; i++)
	{
		out_hpf_signal[i] = hpf_conv_signal[i + HPF_PADDING_SIZE];
		// printf("lpf_signal[%d]: %.15f\n", i, lpf_signal[i]);
	}

	// check the padding removal
	if (out_hpf_signal[0] != hpf_conv_signal[HPF_PADDING_SIZE] || out_hpf_signal[*out_hpf_signal_len - 1] != hpf_conv_signal[HPF_PADDING_SIZE + *out_hpf_signal_len - 1])
	{
		printf("\nError in highpass_filter(): Padding removal failed, check the padding logic.\n");
		printf("hpf_signal[%d]: %.15f\n", 0, out_hpf_signal[0]);
		printf("hpf_signal_wp[%d]: %.15f\n", HPF_PADDING_SIZE, hpf_conv_signal[HPF_PADDING_SIZE]);
		printf("\nhpf_signal[%d]: %.15f\n", *out_hpf_signal_len - 1, out_hpf_signal[*out_hpf_signal_len - 1]);
		printf("hpf_signal_wp[%d]: %.15f\n", HPF_PADDING_SIZE + *out_hpf_signal_len - 1, hpf_conv_signal[HPF_PADDING_SIZE + *out_hpf_signal_len - 1]);
		return 1;
	}
	return 0;
}

int highpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, int in_signal_len, double *out_conv_signal, int out_conv_signal_len)
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

int apply_padding(double *in_signal, int in_signal_len, double *out_padded_signal, int out_padded_signal_len, int padding_size)
{
	// Ensure the padded_signal buffer is large enough
	if (out_padded_signal_len < in_signal_len + (2 * padding_size))
	{
		printf("\nError: out_padded_signal buffer too small in apply_padding\n");
		return 1;
	}

	// Step 1: Apply PADDING_SIZE padding front(left) and back(right) of signal (Replicating MATLAB's Strategy)
	double start_signal = in_signal[0];
	double end_signal = in_signal[in_signal_len - 1];
	for (int i = 0; i < in_signal_len + (padding_size * 2); i++)
	{
		if (i < padding_size)
		{
			out_padded_signal[i] = start_signal; // First PADDING_SIZE padding to left with start_signal
		}
		else if (i >= padding_size && i < padding_size + in_signal_len)
		{
			out_padded_signal[i] = in_signal[i - padding_size]; // Copying signal to middle of padded_signal
		}
		else
		{
			out_padded_signal[i] = end_signal; // Last PADDING_SIZE padding to right with end_signal
		}
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

			if (x2 <= ARTIFACT_DETECT_WINDOW_WIDTH && x1 > 0)
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

// Function to detect artifacts in a signal window
int detect_artifact(const double *window, int window_size, double threshold)
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
void remove_artifact(double *window, int window_len, int x1, int x2)
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

// Function to compute cubic spline coefficients for a single segment
void compute_single_spline(double x1, double y1, double dy1,
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
double evaluate_spline(double a, double b, double c, double d, double x1, double x2, double x_query)
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
