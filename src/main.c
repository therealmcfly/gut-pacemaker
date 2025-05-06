#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // for close(), usleep()
#include "config.h"
#include "data_init.h"
#include "signal_buffering.h"
#include "mode_select.h"
#include "tcp_server.h"

int static_dataset_mode(int argc, char *argv[])
{
	size_t signal_length;
	int channel_num;
	char file_name[100]; // Buffer for file name
	int cur_data_freq;	 // Buffer for exp data frequency
	// INITIALIZE SAMPLE DATA
	// Sample data loading, channel selection, and downsampling is all handled within the get_sample_data function
	// The function will return a pointer to sample data on success, NULL on error
	double *signal = get_sample_data(argc, argv, &signal_length, &channel_num, file_name, &cur_data_freq);

	if (signal == NULL)
	{
		printf("\nError occured while initializing sample data.\n");

		return 1;
	}

	/*----------------------------------------------------------------------------------*/
	/*----------------------------- SIGNAL BUFFERING -----------------------------------*/
	/*----------------------------------------------------------------------------------*/

	if (signal_buffering(signal, signal_length, &channel_num, file_name, &cur_data_freq))
	{
		printf("\nError occured while buffering signal.\n");

		return 1;
	}

	// Free allocated memory
	if (signal != NULL)
	{
		free(signal);
		signal = NULL; // Avoid double free
	}

	return 0;
}

int main(int argc, char *argv[])
{
	RunMode mode = select_mode();

	switch (mode)
	{
	case MODE_STATIC_DATASET:
		printf("\nRunning in Static Dataset Mode...\n");
		if (static_dataset_mode(argc, argv) != 0)
		{
			printf("\nError occured while running static dataset mode.\n");
		}
		break;
	case MODE_REALTIME_DATASET:

		printf("\nRunning in Real-time Dataset Mode...\n");

		if (run_tcp_server())
		{
			printf("\nError occured while running Real-time Dataset Mode.\n");
		}

		break;
	case MODE_GUT_MODEL:
		printf("\nRunning in Gut Model Mode...\n");

		break;
	default:
		printf("\nUnknown mode.\n");
		return 1;
	}

	printf("\nExiting program...\n\n");
	return 0;
}
