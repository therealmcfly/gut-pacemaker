#include "multithreading.h"

#include "config.h"
#include "shared_data.h"

#include "signal_processing.h"
#include "tcp_server.h"
#include "ring_buffer.h"
#include "signal_buffering.h" // need to be removed when signal_buffering.c migrates to detection_pipeline.c

void reset_flag(bool *flag)
{
	*flag = false;
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

void *process_thread(void *data)
{
	double curr_buff_copy[BUFFER_SIZE];
	SharedData *shared_data = (SharedData *)data;

	int start_sig_idx = 0;

	int activations[ACTIVATIONS_ARRAY_SIZE]; // Buffer for activation indices
	int num_activations = 0;
	while (shared_data->server_fd > 0)
	{
		pthread_mutex_lock(shared_data->mutex);
		while (!shared_data->buffer->rtr_flag)
		{
			printf("\nWaiting for data to be ready...\n");
			pthread_cond_wait(shared_data->ready_to_read_cond, shared_data->mutex);
		}

		printf("\nStart activation detection process for buffer %d...\n", shared_data->buffer_count + 1);
		// take snapshot of buffer
		if (!rb_snapshot(shared_data->buffer, curr_buff_copy, shared_data->buff_overlap_count))
		{
			printf("\nError occured while taking snapshot of buffer.\n");
			pthread_mutex_unlock(shared_data->mutex);
			return NULL;
		}
		pthread_mutex_unlock(shared_data->mutex);

		// Process the buffer
		if (detection_pipeline(curr_buff_copy, shared_data->buffer_count, start_sig_idx, &num_activations, activations))
		{
			printf("\nError occured while processing buffer %d.\n", shared_data->buffer_count + 1);
			pthread_mutex_unlock(shared_data->mutex);
			return NULL;
		}
		start_sig_idx += shared_data->buff_overlap_count;

		printf("Finished activation detection process for buffer %d...\n", shared_data->buffer_count + 1);
	}
	return NULL;
}