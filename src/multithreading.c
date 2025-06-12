#include "multithreading.h"

#include <stdio.h>

#include "config.h"
#include "shared_data.h"

#include "signal_processing.h"
#include "networking.h"
#include "ring_buffer.h"

void unlock_mutex()
{
	printf("%sUnlocking mutex...\n", PT_TITLE);
	pthread_mutex_unlock(shared_data.mutex);
}

void *gut_model_mode_receive_thread(void *data)
{
	printf("%sReception thread started...\n", RT_TITLE);

	if (run_tcp_server(&shared_data) != 0)
	{
		printf("\n%sError occured while connection to realtime dataset server.\n", RT_TITLE);
	}
	return NULL;
}

void *rd_mode_receive_thread(void *data)
{
	printf("%sReception thread started...\n", RT_TITLE);

	if (connect_to_server(&shared_data) != 0)
	{
		printf("\n%sError occured while running TCP server.\n", RT_TITLE);
	}
	return NULL;
}

void *process_thread(void *data)
{
	pthread_mutex_lock(shared_data.mutex);

	printf("%sProcessing thread started. Waiting for client connection...\n", PT_TITLE);
	pthread_cond_wait(shared_data.client_connct_cond, shared_data.mutex);
	printf("%sClient connected. Starting processing...\n", PT_TITLE);

	if (shared_data.socket_fd < 0)
	{
		printf("%sNo socket connection. Exiting process thread...\n", PT_TITLE);
		pthread_mutex_unlock(shared_data.mutex);
		return NULL;
	}

	pthread_mutex_unlock(shared_data.mutex);

	// double curr_buff_copy[BUFFER_SIZE];
	int start_idx = 0;
	int activations[ACTIVATIONS_ARRAY_SIZE]; // Buffer for activation indices
	int num_activations = 0;

	printf("%sSocket fd : %d\n", PT_TITLE, shared_data.socket_fd);
	while (shared_data.socket_fd > 0)
	{

		pthread_mutex_lock(shared_data.mutex);
		while (!shared_data.buffer->rtr_flag)
		{
			printf("\n%sWaiting for buffer to be ready...\n", PT_TITLE);
			fflush(stdout);
			pthread_cond_wait(shared_data.ready_to_read_cond, shared_data.mutex);
			if (shared_data.socket_fd > 0)
			{
				continue; // Continue if socket is still valid
			}
			else
			{
				printf("%sSocket connection lost. Exiting process thread...\n", PT_TITLE);
				pthread_mutex_unlock(shared_data.mutex);
				return NULL; // Exit if socket is invalid
			}
		}

		if (shared_data.socket_fd > 0)
		{
			printf("\n%sStart activation detection process for buffer %d...\n", PT_TITLE, shared_data.buffer_count + 1);

			// Process the buffer
			// mutex is unlocked in processing_pipeline via callback function
			if (processing_pipeline(&shared_data.buffer_count, start_idx, &num_activations, activations, unlock_mutex))
			{
				printf("\n%sError occured while processing buffer %d.\n", PT_TITLE, shared_data.buffer_count + 1);
				return NULL;
			}
			start_idx += shared_data.buff_overlap_count;

			printf("%sFinished activation detection process for buffer %d...\n", PT_TITLE, shared_data.buffer_count + 1);
			continue; // Continue to next iteration to wait for the next buffer
		}
		else
		{
			// If socket is invalid, exit the thread
			printf("%sSocket connection lost. Exiting process thread...\n", PT_TITLE);
			pthread_mutex_unlock(shared_data.mutex);
			break; // Exit if socket is still valid
		}
	}
	printf("%sExiting process thread...\n", PT_TITLE);
	return NULL;
}