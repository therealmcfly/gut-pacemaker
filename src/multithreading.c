#include "multithreading.h"

#include <stdio.h>

#include "config.h"
#include "shared_data.h"

#include "signal_processing.h"
#include "tcp_server.h"
#include "ring_buffer.h"

void unlock_mutex()
{
	printf("%sUnlocking mutex...\n", PT_TITLE);
	pthread_mutex_unlock(shared_data.mutex);
}

void *receive_thread(void *data)
{
	printf("%sReception thread started...\n", RT_TITLE);

	if (run_tcp_server(&shared_data) != 0)
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

	pthread_mutex_unlock(shared_data.mutex);

	// double curr_buff_copy[BUFFER_SIZE];
	int start_idx = 0;
	int activations[ACTIVATIONS_ARRAY_SIZE]; // Buffer for activation indices
	int num_activations = 0;

	while (shared_data.client_fd > 0)
	{

		pthread_mutex_lock(shared_data.mutex);
		while (!shared_data.buffer->rtr_flag)
		{
			printf("\n%sWaiting for data to be ready...\n", PT_TITLE);
			fflush(stdout);
			pthread_cond_wait(shared_data.ready_to_read_cond, shared_data.mutex);
		}

		printf("\n%sStart activation detection process for buffer %d...\n", PT_TITLE, shared_data.buffer_count + 1);

		// Process the buffer
		// mutex is unlocked in processing_pipeline via callback function
		if (processing_pipeline(shared_data.buffer_count, start_idx, &num_activations, activations, unlock_mutex))
		{
			printf("\n%sError occured while processing buffer %d.\n", PT_TITLE, shared_data.buffer_count + 1);
			return NULL;
		}
		start_idx += shared_data.buff_overlap_count;

		printf("%sFinished activation detection process for buffer %d...\n", PT_TITLE, shared_data.buffer_count + 1);
	}
	return NULL;
}