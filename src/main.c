#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "config.h"
#include "data_init.h"
#include "signal_buffering.h"

int main(int argc, char *argv[])
{
	size_t signal_length;

	// INITIALIZE SAMPLE DATA
	// Sample data loading, channel selection, and downsampling is all handled within the get_sample_data function
	// The function will return a pointer to sample data on success, NULL on error
	float *signal = get_sample_data(argc, argv, &signal_length);

	if (signal == NULL)
	{
		printf("Error occured while initializing sample data.\n");
		printf("Exiting program...\n");
		return 1;
	}
#if DEBUG
	printf("Sample data[%d]: %f\n", 0, signal[0]);
	printf("Sample data[%d]: %f\n", 1, signal[1]);
	printf("Sample data[%d]: %f\n", 2, signal[2]);
	printf("Data length: %lld\n", signal_length);
#endif

	/*----------------------------------------------------------------------------------*/
	/*----------------------------- SIGNAL BUFFERING -----------------------------------*/
	/*----------------------------------------------------------------------------------*/

	signal_buffering(signal, signal_length);

	return 0;
}