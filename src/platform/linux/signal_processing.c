#include "signal_processing.h"
#include "preprocessing.h"
#include "detection.h"
#include "result_check.h"
#include "global.h"

#include <stdio.h>

#define SUCCESS 0
#define ERROR 1
#define ERROR_BUFFER_SIZE 2

// top of file
static double lpf_sig_buffer[SIGNAL_PROCESSING_BUFFER_SIZE];
static double hpf_sig_buffer[HPF_AD_SIGNAL_LEN];
static double neo_sig_buffer[NEO_MAF_ED_SIGNAL_SIZE];
static double maf_signal[NEO_MAF_ED_SIGNAL_SIZE];
static double edge_sig_buffer[NEO_MAF_ED_SIGNAL_SIZE];

int detect_activations(double *in_signal, size_t signal_length, int *channel, char *filename, int *cur_data_freq)
{
	int activations[ACTIVATIONS_ARRAY_SIZE]; // Buffer for activation indices
	int num_activations = 0;								 // Number of activations
	// if (SIGNAL_PROCESSING_BUFFER_SIZE % 2 != 0)
	// {
	// 	printf("\nError: SIGNAL_PROCESSING_BUFFER_SIZE must be an even number.\n");
	// 	return ERROR_BUFFER_SIZE;
	// }

	// Variables for buffering
	int i = 0, j = SIGNAL_PROCESSING_BUFFER_SIZE, shift = 0;
	// double buffer[MIRROR_MATLAB_LOGIC ? SIGNAL_PROCESSING_BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/ : SIGNAL_PROCESSING_BUFFER_SIZE];
	// double lpf_sig_buffer[MIRROR_MATLAB_LOGIC ? SIGNAL_PROCESSING_BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/ : SIGNAL_PROCESSING_BUFFER_SIZE];
	// double hpf_signal[MIRROR_MATLAB_LOGIC ? (SIGNAL_PROCESSING_BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/) + HPF_FILTER_ORDER : SIGNAL_PROCESSING_BUFFER_SIZE + HPF_FILTER_ORDER];
	// double artifact_signal[MIRROR_MATLAB_LOGIC ? SIGNAL_PROCESSING_BUFFER_SIZE + 1 /*+1 is to mirror the MATLAB logic*/ + HPF_FILTER_ORDER : SIGNAL_PROCESSING_BUFFER_SIZE + HPF_FILTER_ORDER];
	// int cur_buffer_size = SIGNAL_PROCESSING_BUFFER_SIZE; // mirroring of the MATLAB logic, romove if not needed and replace all cur_buffer_size with SIGNAL_PROCESSING_BUFFER_SIZE

	printf("\nStarting signal buffering...\n");
	printf("Signal length: %zu\n", signal_length);

	int last_sample_index = 0;
	while (j < signal_length) // Keep processing until reaching the signal length
	{

		// this is a modification to mirror the logic happening in the MATLAB project. 1st buffer size is 1000 in the first buffer and 1001 in all the rest of the buffers. remove if not needed
		// cur_buffer_size = MIRROR_MATLAB_LOGIC && (shift == 0) ? SIGNAL_PROCESSING_BUFFER_SIZE : SIGNAL_PROCESSING_BUFFER_SIZE + 1;
		last_sample_index = j - 1;
		printf("\nProcessing buffer %d, signal %d to %d ...\n", shift + 1, i, last_sample_index);

		// Copy Original Signal into Buffer
		for (int k = 0; k < SIGNAL_PROCESSING_BUFFER_SIZE; k++)
		{
			if (!rb_push_sample(g_shared_data.buffer, in_signal[i + k])) // Push sample to ring buffer
			{
				printf("\nError: Failed to push sample to ring buffer at index %d.\n", i + k);
				return ERROR;
			}
		}

		if (processing_pipeline(&shift, i, &num_activations, activations, NULL))
		{
			printf("\nError occured in detect activations.\n");

			return 1;
		}
		// Buffer Shift
		shift++;
		i += SIGNAL_PROCESSING_BUFFER_SIZE / 2;
		j = i + SIGNAL_PROCESSING_BUFFER_SIZE;
	}

	// Check Pre Activation Detection Result
#ifdef PRE_ACTIVATION_DETECTION_VERIFICATION
	// int activations_len = sizeof(activations) / sizeof(activations[0]);

	if (check_activations(activations, num_activations, *channel, filename, "actdpre"))
	{
		printf("\nError occured while checking activation detection result.\n");
		return ERROR;
	}
	printf("Pre activation detection verification successful.\n");
#endif

	cleanup_activation_locs(activations, &num_activations, signal_length, CLOSE_PROX_ACT_REMOVAL_THRESHOLD);

#ifdef ACTIVATION_DETECTION_VERIFICATION
	// Check Activation Removal Result
	if (check_activations(activations, num_activations, *channel, filename, "actd"))
	{
		printf("\nError occured while checking activation detection result.\n");
		return ERROR;
	}
	printf("SUCCESS: Activation detection verification successful.\n");
#endif

	printf("\n--------------------- RESULTS ---------------------\n\n");
	printf("Signal name: %s\n", filename);
	printf("Channel number: %d\n", *channel);
	printf("Number of samples: %zu\n", signal_length);
	printf("Number of samples processed: %d\n", last_sample_index);
	printf("Number of activations detected: %d\n", num_activations);
	// Print all activation indices
	printf("\nDetected Activations\n");
	for (int k = 0; k < num_activations; k++)
	{
		printf("Activation %d in index %d\n", k + 1, activations[k]);
	}
	printf("\n---------------------------------------------------\n");

	return SUCCESS;
}

int processing_pipeline(int *shift, int i, int *num_activations, int *activations, void (*callback_unlock_mutex)(void))
{
	printf("\n\tStart detection pipeline for buffer %d...\n", *shift + 1);
	// print the address of shift

	char *filename = file_name;
	int channel = channel_num;
	int freq = cur_data_freq;
	// start the timer_ms for the current buffer
	Timer buffer_timer;
	timer_start(&buffer_timer);

	// this is a modification to mirror the logic happening in the MATLAB project. 1st buffer size is 1000 in the first buffer and 1001 in all the rest of the buffers. remove if not needed
	// cur_buffer_size = MIRROR_MATLAB_LOGIC && (*shift == 0) ? SIGNAL_PROCESSING_BUFFER_SIZE : SIGNAL_PROCESSING_BUFFER_SIZE + 1;

	/* -----------------------------------------------------------------------------*/
	/*                                 PREPROCESSING                                */
	/* -----------------------------------------------------------------------------*/

	/* ---------------------------- Low-Pass Filtering ---------------------------  */
	int lpf_signal_len = sizeof(lpf_sig_buffer) / sizeof(lpf_sig_buffer[0]);
	int is_bad_signal = 0; // 0 means good signal, 1 means bad signal, set dynamically based on the filename below
	// check filename if it starts with "exp" then is_bad_signal = 1, this makes this module not reuseable. need to be changed in the future
	if (filename[0] == 'e' && filename[1] == 'x' && filename[2] == 'p')
	{
		is_bad_signal = 1;
	}

	if (lowpass_filter(lpf_sig_buffer, lpf_signal_len, is_bad_signal, callback_unlock_mutex))
	{
		printf("\nError: Low-pass filtering failed in %dth buffer.\n", *shift);
		return ERROR;
	}

#if DEBUG
	printf("\n%dth signal buffer low pass filtering successful.\n", *shift + 1);
	printf("lpf_signal[%d] = %.15f\n", 0, lpf_sig_buffer[0]);
	printf("lpf_signal[%d] = %.15f\n", 1, lpf_sig_buffer[1]);
	printf("lpf_signal[%d] = %.15f\n", lpf_signal_len - 2, lpf_sig_buffer[lpf_signal_len - 2]);
	printf("lpf_signal[%d] = %.15f\n", lpf_signal_len - 1, lpf_sig_buffer[lpf_signal_len - 1]);
#endif
#ifdef LOW_PASS_FILTER_VERIFICATION
	// Check Low-Pass Filtering Result
	if (check_processing_result(lpf_sig_buffer, lpf_signal_len, channel, filename, "lpf", *shift, PRECISION))
	{
		printf("\nError occured while checking low pass filtering result.\n");
		return ERROR;
	}
	printf("Low-pass filtering successful.\n");
#endif
	printf("\tLow-pass filtering successful.\n");
	/* ---------------------------- High-Pass Filtering ---------------------------  */
	int preprocessed_signal_len = sizeof(hpf_sig_buffer) / sizeof(hpf_sig_buffer[0]);

	if (highpass_filter(lpf_sig_buffer, lpf_signal_len, hpf_sig_buffer, &preprocessed_signal_len))
	{
		printf("\nError: High-pass filtering failed in %dth buffer.\n", *shift);
		return ERROR;
	}
#if DEBUG
	printf("\n%dth signal buffer high pass filtering successful.\n", *shift + 1);
	printf("hpf_signal[%d] = %.15f\n", 0, hpf_sig_buffer[0]);
	printf("hpf_signal[%d] = %.15f\n", 1, hpf_sig_buffer[1]);
	printf("hpf_signal[%d] = %.15f\n", preprocessed_signal_len - 2, hpf_sig_buffer[preprocessed_signal_len - 2]);
	printf("hpf_signal[%d] = %.15f\n", preprocessed_signal_len - 1, hpf_sig_buffer[preprocessed_signal_len - 1]);
#endif
#ifdef HIGH_PASS_FILTER_VERIFICATION
	// Check High-Pass Filtering Result
	if (check_processing_result(hpf_sig_buffer, preprocessed_signal_len, channel, filename, "hpf", *shift, PRECISION))
	{
		printf("\nError occured while checking high pass filtering result.\n");
		return ERROR;
	}
	printf("High-pass filtering verification successful.\n");
#endif
	printf("\tHigh-pass filtering successful.\n");

	/* --------------------- Artifact Detection and Removal ---------------------  */

	if (detect_remove_artifacts(hpf_sig_buffer, preprocessed_signal_len))
	{
		printf("\nError: Artifact detection and removal failed.\n");
		return ERROR;
	}
#ifdef ARTIFACT_REMOVAL_VERIFICATION
	// Check Artifact Removal Result
	if (check_processing_result(hpf_sig_buffer, preprocessed_signal_len, channel, filename, "ad", *shift, PRECISION))
	{
		printf("\nError occured while checking artifact detection and removal result.\n");
		return ERROR;
	}
	printf("Artifact detection and removal verification successful.\n");
#endif
	printf("\tArtifact detection and removal successful.\n");

	/* -----------------------------------------------------------------------------*/
	/*                             ACTIVATION DETECTION                             */
	/* -----------------------------------------------------------------------------*/

	/* -------------------- Non Linear Energy (NEO) Transform --------------------- */
	int neo_signal_len = sizeof(neo_sig_buffer) / sizeof(neo_sig_buffer[0]);

	if (neo_transform(hpf_sig_buffer, preprocessed_signal_len, neo_sig_buffer, neo_signal_len))
	{
		printf("\nError: NEO Transform failed.\n");
		return ERROR;
	}
#ifdef NEO_TRANSFORM_VERIFICATION
	// Check NEO Transform Result
	if (check_processing_result(neo_sig_buffer, neo_signal_len, channel, filename, "neo", *shift, PRECISION))
	{
		printf("\nError occured while checking NEO Transform result.\n");
		return ERROR;
	}
	printf("NEO Transform verification successful.\n");
#endif
	printf("\tNEO Transform successful.\n");

	/* -------------------------- Moving Average Filtering -------------------------- */

	int maf_signal_len = sizeof(maf_signal) / sizeof(maf_signal[0]);
	if (moving_average_filtering(neo_sig_buffer, maf_signal, maf_signal_len, &freq))
	{
		printf("\nError: Moving average filtering failed.\n");
		return ERROR;
	}
#ifdef MOVING_AVERAGE_FILTER_VERIFICATION
	// Check Moving Average Filtering Result
	if (check_processing_result(maf_signal, maf_signal_len, channel, filename, "maf", *shift, PRECISION))
	{
		printf("\nError occured while checking moving average filtering result.\n");
		return ERROR;
	}
	printf("Moving average filtering verification successful.\n");
#endif
	printf("\tMoving average filtering successful.\n");

	/* ---------------------------- Edge Detection ---------------------------- */

	int edge_signal_len = sizeof(edge_sig_buffer) / sizeof(edge_sig_buffer[0]);

	if (edge_detection(hpf_sig_buffer, preprocessed_signal_len, maf_signal, maf_signal_len, edge_sig_buffer, edge_signal_len))
	{
		printf("\nError: Edge detection failed.\n");
		return ERROR;
	}

#ifdef EDGE_DETECTION_VERIFICATION
	// Check Edge Detection Result
	if (check_processing_result(edge_sig_buffer, maf_signal_len, channel, filename, "ed", *shift, ED_PRECISION))
	{
		printf("\nError occured while checking edge detection result.\n");
		return ERROR;
	}
	printf("Edge detection verification successful.\n");
#endif
	printf("\tEdge detection successful.\n");

	/* --------------------------- Activation Detection --------------------------- */

	int buff_activation_indices[BUFFER_ACTIVATION_ARRAY_SIZE];
	int buff_num_activations = 0;

	if (detect_activation(edge_sig_buffer, edge_signal_len, buff_activation_indices, &buff_num_activations, i))
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
		printf("%d activations detected in buffer %d.\n", buff_num_activations, *shift + 1);
		// print activations
		printf("\tActivations: ");
		for (int k = 0; k < buff_num_activations; k++)
		{
			printf("%d ", buff_activation_indices[k]);
		}
		printf("\n");
	}
	else
	{
		printf("No activations detected in buffer %d.\n", *shift + 1);
	}
	printf("\tActivation detection successful!\n");

	// Print all activation indices

	// printf("\nActivation Indices:\n");
	// for (int k = 0; k < *num_activations; k++)
	// {
	// 	printf("activation_indices[%d]: %d\n", k, activations[k]);
	// }

	/* -----------------------------------------------------------------------------*/
	timer_stop(&buffer_timer);
	double elapsed_ms = timer_elapsed_ms(&buffer_timer);
	printf("\n\tSuccessfully finished detection pipeline process!");
	printf(" (Processing time: %.3f ms)\n\n", elapsed_ms);

	return SUCCESS;
}