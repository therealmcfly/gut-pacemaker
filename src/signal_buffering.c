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
	double buffer[BUFFER_SIZE];
	double lowpass_signal[BUFFER_SIZE];

	while (j < signal_length) // Keep processing until reaching the signal length
	{
		// Copy Original Signal into Buffer
		for (int k = 0; k < BUFFER_SIZE; k++)
		{
			buffer[k] = in_signal[i + k];
		}

		// Apply Low-Pass Filtering
		if (lowpass_filter(buffer, lowpass_signal, BUFFER_SIZE))
		{
			printf("Error: Low-pass filtering failed.\n");
			return ERROR;
		}

		// Buffer Shift
		shift++;
		i += BUFFER_SIZE / 2;
		j = i + BUFFER_SIZE;
	}
	printf("Signal buffering completed.\n");
	return SUCCESS;
}
