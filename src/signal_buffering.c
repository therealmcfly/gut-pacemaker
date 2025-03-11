#include "signal_buffering.h"

int signal_buffering(double *in_signal, size_t signal_length)
{
	if (BUFFER_SIZE % 2 != 0)
	{
		printf("Error: BUFFER_SIZE must be an even number.\n");
		return ERROR_BUFFER_SIZE;
	}

	// Variables for buffering
	size_t i = 0, j = BUFFER_SIZE, shift = 0;
	double buffer[BUFFER_SIZE + 1];					// +1 is to mirror the MATLAB logic, romove if not needed
	double lowpass_signal[BUFFER_SIZE + 1]; // +1 is to mirror the MATLAB logic, romove if not needed

	while (j < signal_length) // Keep processing until reaching the signal length
	{
		// Copy Original Signal into Buffer
		for (int k = 0; k < BUFFER_SIZE + 1; k++)
		{
			buffer[k] = in_signal[i + k];
		}

		// this is a modification to mirror the logic happening in the MATLAB project. 1st buffer size is 1000 in the first buffer and 1001 in all the rest of the buffers.
		int cur_buffer_size = BUFFER_SIZE; // mirroring of the MATLAB logic, romove if not needed
		if (shift > 0)										 // mirroring of the MATLAB logic, romove if not needed
			cur_buffer_size++;

		// Apply Low-Pass Filtering
		if (lowpass_filter(buffer, lowpass_signal, cur_buffer_size /* <== is to mirroring of the MATLAB logic, if not needed, change to BUFFER_SIZE*/))
		{
			printf("Error: Low-pass filtering failed.\n");
			return ERROR;
		}
		printf("\n%lldth signal buffer low pass filtering successful.\n", shift + 1);
#if DEBUG
		printf("lpf_signal[%d] = %.15f\n", 0, lowpass_signal[0]);
		printf("lpf_signal[%d] = %.15f\n", 1, lowpass_signal[1]);
		printf("lpf_signal[%d] = %.15f\n", BUFFER_SIZE - 2, lowpass_signal[BUFFER_SIZE - 2]);
		printf("lpf_signal[%d] = %.15f\n", BUFFER_SIZE - 1, lowpass_signal[BUFFER_SIZE - 1]);
		if (shift > 0) // to check the mirroring of the MATLAB logic, romove if not needed
			printf("lpf_signal[%d] = %.15f\n", BUFFER_SIZE, lowpass_signal[BUFFER_SIZE]);
#endif

		// Buffer Shift
		shift++;
		i += BUFFER_SIZE / 2;
		j = i + BUFFER_SIZE;
	}
	return SUCCESS;
}
