#include "mode_select.h"
#include <stdio.h>
#include "config.h"
#include "data_init.h"
#include "signal_buffering.h"
#include "tcp_server.h"
#include "timer_util.h"
#include <pthread.h>

RunMode select_mode(void)
{
	int choice;
	while (1)
	{
		printf("\nWelcome to Gut Pacemaker!\n");
		printf("\nPlease select a mode:\n");
		printf("\n1. Static Dataset Mode\n");
		printf("2. Real-time Dataset Mode\n");
		printf("3. Gut Model Mode\n");
		printf("\nEnter choice (1-3): ");
		if (scanf("%d", &choice) != 1)
		{
			while (getchar() != '\n')
				; // clear stdin buffer
			printf("Invalid input. Please enter a number.\n");
			continue;
		}
		if (choice >= 1 && choice <= 3)
			return (RunMode)choice;
		else
			printf("Invalid choice. Try again.\n");
	}
}

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

int realtime_dataset_mode(int argc, char *argv[])
{
	pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;
	int client_active = 1;
	RingBufferDouble cir_buffer;

	run_tcp_server(&cir_buffer);
	return 0;
}