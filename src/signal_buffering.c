#include "signal_buffering.h"

int load_coeffs(char *filename, float *lowpass_coeffs)
{
	char file_path[200];
	snprintf(file_path, sizeof(file_path), "%s%s", DATA_DIRECTORY, filename);
	FILE *file = fopen(file_path, "r");
	if (!file)
	{
		printf("\nError: Could not open coefficient file %s\n", file_path);
		return 1;
	}

	for (int i = 0; i < FILTER_ORDER; i++)
	{
		fscanf(file, "%f", &lowpass_coeffs[i]);
	}

	printf("Low-pass filter coefficients loaded.\n");
	fclose(file);
	return 0;
}

void signal_buffering(float *in_signal, size_t signal_length)
{

	// Variables for buffering
	int i = 0, j = BUFFER_SIZE, shift = 0;
	int activation_index = 0;
	float buffer[BUFFER_SIZE];
	float lowpass_coeffs[FILTER_ORDER + 1];
	float lowpass_signal[BUFFER_SIZE];
	// float highpass_signal[BUFFER_SIZE];
	// float artifacts_removed[BUFFER_SIZE];

	// load low-pass filter coefficients
	if (load_coeffs(COEFFICIENTS_FILE, lowpass_coeffs))
	{
		printf("Error: Failed to load low-pass filter coefficients.\n");
	}

	// The processing loop will iterate over the signal array, processing BUFFER_SIZE samples at a time
	while (j < signal_length) // compairing int with size_t? need to check if this is correct
	{
		for (int k = 0; k < BUFFER_SIZE; k++)
		{
			buffer[k] = in_signal[i + k];
		}

		// **Low-pass Filtering**
		lowpass_filter(buffer, lowpass_signal, BUFFER_SIZE, lowpass_coeffs, FILTER_ORDER);

		// **High-pass Filtering**

		// **Buffer Shift**
		shift++;
		i += BUFFER_SIZE / 2;
		j = i + BUFFER_SIZE;
	}
}
