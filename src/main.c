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
	double *signal = get_sample_data(argc, argv, &signal_length);

	if (signal == NULL)
	{
		printf("Error occured while initializing sample data.\n");
		printf("Exiting program...\n");
		return 1;
	}

	/*----------------------------------------------------------------------------------*/
	/*----------------------------- SIGNAL BUFFERING -----------------------------------*/
	/*----------------------------------------------------------------------------------*/

	if (signal_buffering(signal, signal_length))
	{
		printf("Error occured while buffering signal.\n");
		printf("Exiting program...\n");
		return 1;
	}
	printf("Exiting program...\n");
	return 0;
}