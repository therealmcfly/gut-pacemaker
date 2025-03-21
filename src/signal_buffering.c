#include "signal_buffering.h"

int signal_buffering(double *in_signal, size_t signal_length, int *channel_num, char *file_name)
{
	// if (BUFFER_SIZE % 2 != 0)
	// {
	// 	printf("\nError: BUFFER_SIZE must be an even number.\n");
	// 	return ERROR_BUFFER_SIZE;
	// }

	// Variables for buffering
	int i = 0, j = BUFFER_SIZE, shift = 0;
	// double buffer[MIRROR_MATLAB_LOGIC ? BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/ : BUFFER_SIZE];
	// double lowpass_signal[MIRROR_MATLAB_LOGIC ? BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/ : BUFFER_SIZE];
	// double hpf_signal[MIRROR_MATLAB_LOGIC ? (BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/) + HPF_FILTER_ORDER : BUFFER_SIZE + HPF_FILTER_ORDER];
	// double artifact_signal[MIRROR_MATLAB_LOGIC ? BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/ + HPF_FILTER_ORDER : BUFFER_SIZE + HPF_FILTER_ORDER];
	double buffer[BUFFER_SIZE];
	// int cur_buffer_size = BUFFER_SIZE; // mirroring of the MATLAB logic, romove if not needed and replace all cur_buffer_size with BUFFER_SIZE

	while (j < signal_length) // Keep processing until reaching the signal length
	{
		// this is a modification to mirror the logic happening in the MATLAB project. 1st buffer size is 1000 in the first buffer and 1001 in all the rest of the buffers. remove if not needed
		// cur_buffer_size = MIRROR_MATLAB_LOGIC && (shift == 0) ? BUFFER_SIZE : BUFFER_SIZE + 1;

		printf("\nProcessing buffer %d...\n", shift + 1);
		// Copy Original Signal into Buffer
		for (int k = 0; k < BUFFER_SIZE; k++)
		{
			buffer[k] = in_signal[i + k];
		}

		/* -----------------------------------------------------------------------------*/
		/*                                 PREPROCESSING                                */
		/* -----------------------------------------------------------------------------*/

		/* ---------------------------- Low-Pass Filtering ---------------------------  */
		double lowpass_signal[BUFFER_SIZE];
		int lpf_signal_len = sizeof(lowpass_signal) / sizeof(lowpass_signal[0]);

		if (lowpass_filter(buffer, lowpass_signal, lpf_signal_len))
		{
			printf("\nError: Low-pass filtering failed.\n");
			return ERROR;
		}
#if DEBUG
		printf("\n%dth signal buffer low pass filtering successful.\n", shift + 1);
		printf("lpf_signal[%d] = %.15f\n", 0, lowpass_signal[0]);
		printf("lpf_signal[%d] = %.15f\n", 1, lowpass_signal[1]);
		printf("lpf_signal[%d] = %.15f\n", lpf_signal_len - 2, lowpass_signal[lpf_signal_len - 2]);
		printf("lpf_signal[%d] = %.15f\n", lpf_signal_len - 1, lowpass_signal[lpf_signal_len - 1]);
#endif
#if LOW_PASS_FILTER_VERIFICATION
		// Check Low-Pass Filtering Result
		if (check_processing_result(lowpass_signal, lpf_signal_len, *channel_num, file_name, "lpf", shift))
		{
			printf("\nError occured while checking low pass filtering result.\n");
			return ERROR;
		}
		printf("Low-pass filtering successful.\n");
#endif

		/* ---------------------------- High-Pass Filtering ---------------------------  */
		double preprocessed_signal[HPF_AD_SIGNAL_LEN];
		int preprocessed_signal_len = sizeof(preprocessed_signal) / sizeof(preprocessed_signal[0]);

		if (highpass_filter(lowpass_signal, lpf_signal_len, preprocessed_signal, &preprocessed_signal_len))
		{
			printf("\nError: High-pass filtering failed in %dth buffer.\n", shift);
			return ERROR;
		}
#if DEBUG
		printf("\n%dth signal buffer high pass filtering successful.\n", shift + 1);
		printf("hpf_signal[%d] = %.15f\n", 0, preprocessed_signal[0]);
		printf("hpf_signal[%d] = %.15f\n", 1, preprocessed_signal[1]);
		printf("hpf_signal[%d] = %.15f\n", preprocessed_signal_len - 2, preprocessed_signal[preprocessed_signal_len - 2]);
		printf("hpf_signal[%d] = %.15f\n", preprocessed_signal_len - 1, preprocessed_signal[preprocessed_signal_len - 1]);
#endif
#if HIGH_PASS_FILTER_VERIFICATION
		// Check High-Pass Filtering Result
		if (check_processing_result(preprocessed_signal, preprocessed_signal_len, *channel_num, file_name, "hpf", shift))
		{
			printf("\nError occured while checking high pass filtering result.\n");
			return ERROR;
		}
		printf("High-pass filtering successful.\n");
#endif

		/* --------------------- Artifact Detection and Removal ---------------------  */

		if (detect_remove_artifacts(preprocessed_signal, preprocessed_signal_len))
		{
			printf("\nError: Artifact detection and removal failed.\n");
			return ERROR;
		}
#if ARTIFACT_REMOVAL_VERIFICATION
		// Check Artifact Removal Result
		if (check_processing_result(preprocessed_signal, preprocessed_signal_len, *channel_num, file_name, "ad", shift))
		{
			printf("\nError occured while checking artifact detection and removal result.\n");
			return ERROR;
		}
		printf("Artifact detection and removal successful.\n");
#endif

		/* -----------------------------------------------------------------------------*/
		/*                             ACTIVATION DETECTION                             */
		/* -----------------------------------------------------------------------------*/

		/* -------------------- Non Linear Energy (NEO) Transform --------------------- */
		double neo_signal[NEO_MAF_ED_SIGNAL_SIZE]; // NEO Transform output signal is 1 less than the input signal
		int neo_signal_len = sizeof(neo_signal) / sizeof(neo_signal[0]);
		if (neo_transform(preprocessed_signal, preprocessed_signal_len, neo_signal, neo_signal_len))
		{
			printf("\nError: NEO Transform failed.\n");
			return ERROR;
		}
#if NEO_TRANSFORM_VERIFICATION
		// Check NEO Transform Result
		if (check_processing_result(neo_signal, neo_signal_len, *channel_num, file_name, "neo", shift))
		{
			printf("\nError occured while checking NEO Transform result.\n");
			return ERROR;
		}
		printf("NEO Transform successful.\n");
#endif

		/* -------------------------- Moving Average Filtering -------------------------- */

		double maf_signal[NEO_MAF_ED_SIGNAL_SIZE];
		int maf_signal_len = sizeof(maf_signal) / sizeof(maf_signal[0]);

		if (moving_average_filtering(neo_signal, maf_signal, maf_signal_len, TARGET_FREQUENCY))
		{
			printf("\nError: Moving average filtering failed.\n");
			return ERROR;
		}
#if MOVING_AVERAGE_FILTER_VERIFICATION
		// Check Moving Average Filtering Result
		if (check_processing_result(maf_signal, maf_signal_len, *channel_num, file_name, "maf", shift))
		{
			printf("\nError occured while checking moving average filtering result.\n");
			return ERROR;
		}
		printf("Moving average filtering successful.\n");
#endif

		/* ---------------------------- Edge Detection ---------------------------- */

		double edge_signal[NEO_MAF_ED_SIGNAL_SIZE];
		int edge_signal_len = sizeof(edge_signal) / sizeof(edge_signal[0]);

		if (edge_detection(preprocessed_signal, preprocessed_signal_len, maf_signal, maf_signal_len, edge_signal, edge_signal_len))
		{
			printf("\nError: Edge detection failed.\n");
			return ERROR;
		}

#if EDGE_DETECTION_VERIFICATION
		// Check Edge Detection Result
		if (check_processing_result(edge_signal, maf_signal_len, *channel_num, file_name, "ed", shift))
		{
			printf("\nError occured while checking edge detection result.\n");
			return ERROR;
		}
		printf("Edge detection successful.\n");
#endif

		if (shift == 0)
			break;
		/* -----------------------------------------------------------------------------*/

		// Buffer Shift
		shift++;
		i += BUFFER_SIZE / 2;
		j = i + BUFFER_SIZE;
	}

	return SUCCESS;
}
