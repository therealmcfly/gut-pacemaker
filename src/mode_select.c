#include "mode_select.h"
#include <stdlib.h> /* malloc, free */
#include <stdio.h>
#include <pthread.h>
#include "config.h"
#include "data_init.h"
#include "signal_buffering.h"
#include "tcp_server.h"
#include "timer_util.h"
#include "shared_data.h"
#include "detection_pipeline.h"

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

void *process_thread(void *data)
{
	double curr_buff_copy[BUFFER_SIZE];
	SharedData *shared_data = (SharedData *)data;

	int start_sig_idx = 0;
	int *sig_count = shared_data->sig_process_count;

	int activations[ACTIVATIONS_ARRAY_SIZE]; // Buffer for activation indices
	int num_activations = 0;
	while (shared_data->server_fd > 0)
	{
		pthread_mutex_lock(shared_data->mutex);
		while (!shared_data->buffer->ready_to_read)
		{
			printf("\nWaiting for data to be ready...\n");
			pthread_cond_wait(shared_data->cond, shared_data->mutex);
		}

		shared_data->buffer->ready_to_read = false;
		rb_snapshot(shared_data->buffer, curr_buff_copy, shared_data->buff_overlap_count);
		pthread_mutex_unlock(shared_data->mutex);
		printf("Signal processing data for buffer %d...\n", *sig_count + 1);

		// Process the buffer
		if (detection_pipeline(curr_buff_copy, *sig_count, start_sig_idx, &num_activations, activations))
		{
			printf("\nError occured while processing buffer %d.\n", *sig_count + 1);
			return NULL;
		}
		start_sig_idx += shared_data->buff_overlap_count;

		printf("Finished processing buffer %d...\n", *sig_count + 1);
		(*sig_count)++;
	}
	return NULL;
}

void *receive_thread(void *data)
{
	SharedData *shared_data = (SharedData *)data;
	if (run_tcp_server(shared_data) != 0)
	{
		printf("\nError occured while running TCP server.\n");
	}
	return NULL;
}

int realtime_dataset_mode(int argc, char *argv[])
{

	// Initialize mutex and condition variable
	pthread_mutex_t buffer_mutex;
	pthread_cond_t buffer_ready;
	pthread_mutex_init(&buffer_mutex, NULL);
	pthread_cond_init(&buffer_ready, NULL);

	// Initialize ring buffer
	RingBuffer cir_buffer;
	rb_init(&cir_buffer);

	int sig_process_count = 0;

	// Initialize shared data
	SharedData shared_data = {
			.buffer = &cir_buffer,
			.mutex = &buffer_mutex,
			.cond = &buffer_ready,
			.server_fd = -1,
			.client_fd = -1,
			.buffer_initial_fill = false,
			.buff_overlap_count = BUFFER_SIZE_HALF,
			.sig_process_count = &sig_process_count};

	pthread_t recv_thtread, proc_thread;

	if (pthread_create(&recv_thtread, NULL, receive_thread, &shared_data) != 0)
	{
		printf("\nError creating TCP server thread.\n");
		return 1;
	}

	if (pthread_create(&proc_thread, NULL, process_thread, &shared_data) != 0)
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
	pthread_cond_destroy(&buffer_ready);

	return 0;
}

int gut_model_mode(int argc, char *argv[])
{
	printf("\nGut Model Mode is not implemented yet.\n");
	return 0;
}