#include "signal_buffering.h"
#include "file_io.h"
#include <string.h>

int signal_buffering(double *in_signal, size_t signal_length, int *channel_num)
{
	if (BUFFER_SIZE % 2 != 0)
	{
		printf("\nError: BUFFER_SIZE must be an even number.\n");
		return ERROR_BUFFER_SIZE;
	}

	// Variables for buffering
	int i = 0, j = BUFFER_SIZE, shift = 0;
	double buffer[BUFFER_SIZE + 1];					// +1 is to mirror the MATLAB logic, romove if not needed
	double lowpass_signal[BUFFER_SIZE + 1]; // +1 is to mirror the MATLAB logic, romove if not needed

	// Load low pass results from Daryl's MATLAB code
	size_t mat_data_rows, mat_data_cols;
	char ver_filepath[200];
	// Create a mutable copy of the filename to safely tokenize it.
	char filename_copy[20] = "exp_16_output";
	sprintf(ver_filepath, "%sver_lpf_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(filename_copy, "."), *channel_num);
	double **lpf_mat_data = import_file(ver_filepath, &mat_data_rows, &mat_data_cols);
	sprintf(ver_filepath, "%sver_hpf_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(filename_copy, "."), *channel_num);
	double **hpf_mat_data = import_file(ver_filepath, &mat_data_rows, &mat_data_cols);

	while (j < signal_length) // Keep processing until reaching the signal length
	{
		printf("\nProcessing buffer %d...\n", shift + 1);
		// Copy Original Signal into Buffer
		for (int k = 0; k < BUFFER_SIZE + 1; k++)
		{
			buffer[k] = in_signal[i + k];
		}

		// this is a modification to mirror the logic happening in the MATLAB project. 1st buffer size is 1000 in the first buffer and 1001 in all the rest of the buffers.
		int cur_buffer_size = BUFFER_SIZE; // mirroring of the MATLAB logic, romove if not needed
		if (shift > 0)										 // mirroring of the MATLAB logic, romove if not needed
			cur_buffer_size++;

		/* ----------------------------------------------------------------------------------------------------*/
		/* --------------------------------------- LOW PASS FILTERING -----------------------------------------*/
		/* ----------------------------------------------------------------------------------------------------*/

		// Apply Low-Pass Filtering
		if (lowpass_filter(buffer, lowpass_signal, cur_buffer_size /* <== is to mirroring of the MATLAB logic, if not needed, change to BUFFER_SIZE*/))
		{
			printf("Error: Low-pass filtering failed.\n");
			return ERROR;
		}
#if DEBUG
		printf("\n%dth signal buffer low pass filtering successful.\n", shift + 1);
		printf("lpf_signal[%d] = %.15f\n", 0, lowpass_signal[0]);
		printf("lpf_signal[%d] = %.15f\n", 1, lowpass_signal[1]);
		printf("lpf_signal[%d] = %.15f\n", BUFFER_SIZE - 2, lowpass_signal[BUFFER_SIZE - 2]);
		printf("lpf_signal[%d] = %.15f\n", BUFFER_SIZE - 1, lowpass_signal[BUFFER_SIZE - 1]);
		if (shift > 0) // to check the mirroring of the MATLAB logic, romove if not needed
			printf("lpf_signal[%d] = %.15f\n", BUFFER_SIZE, lowpass_signal[BUFFER_SIZE]);
#endif
		// Check results
		for (int k = 0; k < BUFFER_SIZE; k++)
		{
			// if two values are the same until the 6th decimal place, they are considered equal

			if ((lowpass_signal[k] - lpf_mat_data[k][shift] > PRECISION) ||
					(lpf_mat_data[k][shift] - lowpass_signal[k] > PRECISION))
			{
				printf("\nError: Low-pass filtering result mismatch at buffer %d, index %d.\n", shift, k);
				printf("lowpass_signal[%d] = %.15f\n", k, lowpass_signal[k]);
				printf("lpf_mat_data[%d][%d] = %.15f\n", shift, k, lpf_mat_data[k][shift]);
				return ERROR;
			}
		}
		printf("Low-pass filtering successful\n");

		/* ----------------------------------------------------------------------------------------------------*/
		/* -------------------------------------- HIGH PASS FILTERING -----------------------------------------*/
		/* ----------------------------------------------------------------------------------------------------*/
		if (highpass_filter(buffer, lowpass_signal, cur_buffer_size /* <== is to mirroring of the MATLAB logic, if not needed, change to BUFFER_SIZE*/))
		{
			printf("\nError: High-pass filtering failed.\n");
			return ERROR;
		}

		// Buffer Shift
		shift++;
		i += BUFFER_SIZE / 2;
		j = i + BUFFER_SIZE;
	}

	// Free allocated memory
	if (lpf_mat_data != NULL)
	{
		for (size_t i = 0; i < mat_data_rows; i++)
		{
			free(lpf_mat_data[i]);
		}
		free(lpf_mat_data);
	}
	if (hpf_mat_data != NULL)
	{
		for (size_t i = 0; i < mat_data_rows; i++)
		{
			free(hpf_mat_data[i]);
		}
		free(hpf_mat_data);
	}

	return SUCCESS;
}
