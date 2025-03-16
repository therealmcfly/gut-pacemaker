#include "signal_buffering.h"

int signal_buffering(double *in_signal, size_t signal_length, int *channel_num, char *file_name)
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
	double hpf_signal[BUFFER_SIZE + 1];			// +1 is to mirror the MATLAB logic, romove if not needed

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
			printf("\nError: Low-pass filtering failed.\n");
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
#if PROCESSING_RESULT_CHECK
		// Check Low-Pass Filtering Result
		if (check_processing_result(lowpass_signal, cur_buffer_size, *channel_num, file_name, "lpf", shift))
		{
			printf("\nError occured while checking low pass filtering result.\n");
			return ERROR;
		}
		printf("Low-pass filtering successful.\n");
#endif

		/* ----------------------------------------------------------------------------------------------------*/
		/* -------------------------------------- HIGH PASS FILTERING -----------------------------------------*/
		/* ----------------------------------------------------------------------------------------------------*/
		if (highpass_filter(lowpass_signal, hpf_signal, cur_buffer_size /* <== is to mirroring of the MATLAB logic, if not needed, change to BUFFER_SIZE*/))
		{
			printf("\nError: High-pass filtering failed.\n");
			return ERROR;
		}
#if DEBUG
		printf("\n%dth signal buffer high pass filtering successful.\n", shift + 1);
		printf("hpf_signal[%d] = %.15f\n", 0, hpf_signal[0]);
		printf("hpf_signal[%d] = %.15f\n", 1, hpf_signal[1]);
		printf("hpf_signal[%d] = %.15f\n", BUFFER_SIZE - 2, hpf_signal[BUFFER_SIZE - 2]);
		printf("hpf_signal[%d] = %.15f\n", BUFFER_SIZE - 1, hpf_signal[BUFFER_SIZE - 1]);
		if (shift > 0) // to check the mirroring of the MATLAB logic, romove if not needed
			printf("hpf_signal[%d] = %.15f\n", BUFFER_SIZE, hpf_signal[BUFFER_SIZE]);
#endif
#if PROCESSING_RESULT_CHECK
		// Check High-Pass Filtering Result
		if (check_processing_result(hpf_signal, cur_buffer_size, *channel_num, file_name, "hpf", shift))
		{
			printf("\nError occured while checking high pass filtering result.\n");
			return ERROR;
		}
		printf("High-pass filtering successful.\n");
#endif

		// Buffer Shift
		shift++;
		i += BUFFER_SIZE / 2;
		j = i + BUFFER_SIZE;
	}

	return SUCCESS;
}
