#include "mode_select.h"
#include <stdio.h>
#include <pthread.h>
#include "config.h"
#include "data_init.h"
#include "shared_data.h"
#include "multithreading.h"
// #include "signal_buffering.h"
#include "signal_processing.h"

// Initialize global variables
size_t signal_length;
int channel_num;
char file_name[100]; // Buffer for file name
int cur_data_freq;	 // Buffer for exp data frequency

SharedData shared_data; // Global shared data for all threads

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
		{
			while (getchar() != '\n')
			{
				// flush input
			}
			return (RunMode)choice;
		}

		else
			printf("Invalid choice. Try again.\n");
	}
}

int static_dataset_mode(int argc, char *argv[])
{
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

	if (detect_activations(signal, signal_length, &channel_num, file_name, &cur_data_freq))
	{
		printf("\nError occured in static dataset mode.\n");
		if (signal) /* prevent leak on failure */
			free(signal);
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

	// Initialize mutex and condition variable
	pthread_mutex_t buffer_mutex;
	pthread_cond_t client_connct_cond;
	pthread_cond_t ready_to_read_cond;
	pthread_mutex_init(&buffer_mutex, NULL);
	pthread_cond_init(&client_connct_cond, NULL);
	pthread_cond_init(&ready_to_read_cond, NULL);

	// Initialize ring buffer
	RingBuffer cir_buffer;
	rb_init(&cir_buffer);

	// Initialize shared data
	shared_data.buffer = &cir_buffer; // pointer to ring buffer
	shared_data.mutex = &buffer_mutex;
	shared_data.client_connct_cond = &client_connct_cond;
	shared_data.ready_to_read_cond = &ready_to_read_cond;
	shared_data.buffer_count = 0;											 // buffer count
	shared_data.buff_overlap_count = BUFFER_SIZE_HALF; // overlap count
	shared_data.server_fd = -1;												 // server file descriptor
	shared_data.client_fd = -1;												 // client file descriptor

	pthread_t recv_thtread, proc_thread;

	if (pthread_create(&recv_thtread, NULL, receive_thread, NULL) != 0)
	{
		printf("\nError creating TCP server thread.\n");

		return 1;
	}

	if (pthread_create(&proc_thread, NULL, process_thread, NULL) != 0)
	{
		printf("\nError creating signal buffering thread.\n");
		return 1;
	}

	if (pthread_join(recv_thtread, NULL) != 0)
	{
		printf("\nError joining TCP server thread.\n");
		return 1;
	}
	if (pthread_join(proc_thread, NULL) != 0)
	{
		printf("\nError joining signal buffering thread.\n");
		return 1;
	}
	pthread_mutex_destroy(&buffer_mutex);
	pthread_cond_destroy(&ready_to_read_cond);

	return 0;
}

int gut_model_mode(int argc, char *argv[])
{
	printf("\nGut Model Mode is not implemented yet.\n");
	return 0;
}