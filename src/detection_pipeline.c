#include "detection_pipeline.h"
#include "config.h"
#include "timer_util.h"
#include "preprocessing.h"
#include "detection.h"

#include <stdio.h>

#define SUCCESS 0
#define ERROR 1

int detection_pipeline(double *buffer, int shift, int i, int *num_activations, int *activations)
{
	printf("Starting detection pipeline...\n");
	// start the timer for the current buffer
	Timer buffer_timer;
	timer_start(&buffer_timer);

	// this is a modification to mirror the logic happening in the MATLAB project. 1st buffer size is 1000 in the first buffer and 1001 in all the rest of the buffers. remove if not needed
	// cur_buffer_size = MIRROR_MATLAB_LOGIC && (shift == 0) ? BUFFER_SIZE : BUFFER_SIZE + 1;

	/* -----------------------------------------------------------------------------*/
	/*                                 PREPROCESSING                                */
	/* -----------------------------------------------------------------------------*/

	/* ---------------------------- Low-Pass Filtering ---------------------------  */
	double lowpass_signal[BUFFER_SIZE];
	int lpf_signal_len = sizeof(lowpass_signal) / sizeof(lowpass_signal[0]);

	if (lowpass_filter(buffer, lowpass_signal, lpf_signal_len))
	{
		printf("\nError: Low-pass filtering failed in %dth buffer.\n", shift);
		return ERROR;
	}
#if DEBUG
	printf("\n%dth signal buffer low pass filtering successful.\n", shift + 1);
	printf("lpf_signal[%d] = %.15f\n", 0, lowpass_signal[0]);
	printf("lpf_signal[%d] = %.15f\n", 1, lowpass_signal[1]);
	printf("lpf_signal[%d] = %.15f\n", lpf_signal_len - 2, lowpass_signal[lpf_signal_len - 2]);
	printf("lpf_signal[%d] = %.15f\n", lpf_signal_len - 1, lowpass_signal[lpf_signal_len - 1]);
#endif
#ifdef LOW_PASS_FILTER_VERIFICATION
	// Check Low-Pass Filtering Result
	if (check_processing_result(lowpass_signal, lpf_signal_len, *channel_num, file_name, "lpf", shift, PRECISION))
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
#ifdef HIGH_PASS_FILTER_VERIFICATION
	// Check High-Pass Filtering Result
	if (check_processing_result(preprocessed_signal, preprocessed_signal_len, *channel_num, file_name, "hpf", shift, PRECISION))
	{
		printf("\nError occured while checking high pass filtering result.\n");
		return ERROR;
	}
	printf("High-pass filtering verification successful.\n");
#endif
	/* --------------------- Artifact Detection and Removal ---------------------  */

	if (detect_remove_artifacts(preprocessed_signal, preprocessed_signal_len))
	{
		printf("\nError: Artifact detection and removal failed.\n");
		return ERROR;
	}
#ifdef ARTIFACT_REMOVAL_VERIFICATION
	// Check Artifact Removal Result
	if (check_processing_result(preprocessed_signal, preprocessed_signal_len, *channel_num, file_name, "ad", shift, PRECISION))
	{
		printf("\nError occured while checking artifact detection and removal result.\n");
		return ERROR;
	}
	printf("Artifact detection and removal verification successful.\n");
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
#ifdef NEO_TRANSFORM_VERIFICATION
	// Check NEO Transform Result
	if (check_processing_result(neo_signal, neo_signal_len, *channel_num, file_name, "neo", shift, PRECISION))
	{
		printf("\nError occured while checking NEO Transform result.\n");
		return ERROR;
	}
	printf("NEO Transform verification successful.\n");
#endif

	/* -------------------------- Moving Average Filtering -------------------------- */

	double maf_signal[NEO_MAF_ED_SIGNAL_SIZE];
	int maf_signal_len = sizeof(maf_signal) / sizeof(maf_signal[0]);
	int freq = TARGET_FREQUENCY;

	if (moving_average_filtering(neo_signal, maf_signal, maf_signal_len, &freq))
	{
		printf("\nError: Moving average filtering failed.\n");
		return ERROR;
	}
#ifdef MOVING_AVERAGE_FILTER_VERIFICATION
	// Check Moving Average Filtering Result
	if (check_processing_result(maf_signal, maf_signal_len, *channel_num, file_name, "maf", shift, PRECISION))
	{
		printf("\nError occured while checking moving average filtering result.\n");
		return ERROR;
	}
	printf("Moving average filtering verification successful.\n");
#endif

	/* ---------------------------- Edge Detection ---------------------------- */

	double edge_signal[NEO_MAF_ED_SIGNAL_SIZE];
	int edge_signal_len = sizeof(edge_signal) / sizeof(edge_signal[0]);

	if (edge_detection(preprocessed_signal, preprocessed_signal_len, maf_signal, maf_signal_len, edge_signal, edge_signal_len))
	{
		printf("\nError: Edge detection failed.\n");
		return ERROR;
	}

#ifdef EDGE_DETECTION_VERIFICATION
	// Check Edge Detection Result
	if (check_processing_result(edge_signal, maf_signal_len, *channel_num, file_name, "ed", shift, ED_PRECISION))
	{
		printf("\nError occured while checking edge detection result.\n");
		return ERROR;
	}
	printf("Edge detection verification successful.\n");
#endif

	/* --------------------------- Activation Detection --------------------------- */

	int buff_activation_indices[BUFFER_ACTIVATION_ARRAY_SIZE];
	int buff_num_activations = 0;

	if (detect_activation(edge_signal, edge_signal_len, buff_activation_indices, &buff_num_activations, i))
	{
		printf("\nError: Activation detection failed.\n");
		return ERROR;
	}

	if (buff_num_activations)
	{
		if (*num_activations + buff_num_activations > ACTIVATIONS_ARRAY_SIZE)
		{
			printf("\nError: Not enough space in 'activations' array. Number of activations detected (%d) before close proximity removals are greater than the allocated size of the activations array(%d). This will result to unexpected outcomes due to overflow. Please reset ACTIVATIONS_ARRAY_SIZE to higher value in config.h.\n", *num_activations + buff_num_activations, ACTIVATIONS_ARRAY_SIZE);

			return ERROR;
		}
		for (int k = 0; k < buff_num_activations; k++)
		{
			activations[k + *num_activations] = buff_activation_indices[k];
		}
		*num_activations += buff_num_activations;
	}

	// Print all activation indices

	// printf("\nActivation Indices:\n");
	// for (int k = 0; k < *num_activations; k++)
	// {
	// 	printf("activation_indices[%d]: %d\n", k, activations[k]);
	// }

	/* -----------------------------------------------------------------------------*/
	printf("Buffer %d processing successful.\n", shift + 1);
	timer_stop(&buffer_timer);
	double elapsed_ms = timer_elapsed_ms(&buffer_timer);
	printf("Buffer %d processing time: %.3f ms\n", shift + 1, elapsed_ms);

	// // Buffer Shift
	// shift++;
	// i += BUFFER_SIZE_HALF;
	return SUCCESS;
}