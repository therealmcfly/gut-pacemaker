#include "multithreading.h"

#include <stdio.h>
#include "global.h"
#include "signal_processing.h"
#include "networking.h"
#include "ring_buffer.h"
#include "pacemaker.h"

void unlock_mutex()
{
	// printf("%sUnlocking mutex...\n", PT_TITLE);
	pthread_mutex_unlock(g_shared_data.mutex);
}

void *gut_model_mode_receive_thread(void *ch_ptr)
{
	int ch = *(int *)ch_ptr; // Get the channel number from the argument
	printf("%sReception thread started...\n", RT_TITLE);

	if (run_pacemaker_server(&g_shared_data, g_shared_data.ch_datas_prt[ch]) != 0)
	{
		printf("\n%sError occured while running pacemaker server.\n", RT_TITLE);
	}
	return NULL;
}

void *rd_mode_receive_thread(void *data)
{
	printf("%sReception thread started...\n", RT_TITLE);

	if (connect_to_server(&g_shared_data) != 0)
	{
		printf("\n%sError occured while running TCP server.\n", RT_TITLE);
	}
	return NULL;
}

void *process_thread(void *data)
{
	pthread_mutex_lock(g_shared_data.mutex);

	printf("%sProcessing thread started. Waiting for client connection...\n", PT_TITLE);
	pthread_cond_wait(g_shared_data.client_connct_cond, g_shared_data.mutex);
	printf("%sClient connected. Starting processing...\n", PT_TITLE);

	if (g_shared_data.socket_fd < 0)
	{
		printf("%sNo socket connection. Exiting process thread...\n", PT_TITLE);
		pthread_mutex_unlock(g_shared_data.mutex);
		return NULL;
	}

	pthread_mutex_unlock(g_shared_data.mutex);

	// double curr_buff_copy[SIGNAL_PROCESSING_BUFFER_SIZE];
	int start_idx = 0;
	int activations[ACTIVATIONS_ARRAY_SIZE]; // Buffer for activation indices
	int num_activations = 0;

	printf("%sSocket fd : %d\n", PT_TITLE, g_shared_data.socket_fd);
	while (g_shared_data.socket_fd > 0)
	{

		pthread_mutex_lock(g_shared_data.mutex);
		while (!g_shared_data.buffer->rtr_flag)
		{
			printf("\n%sWaiting for buffer to be ready...\n", PT_TITLE);
			fflush(stdout);
			pthread_cond_wait(g_shared_data.ready_to_read_cond, g_shared_data.mutex);
			if (g_shared_data.socket_fd > 0)
			{
				continue; // Continue if socket is still valid
			}
			else
			{
				printf("%sSocket connection lost. Exiting process thread...\n", PT_TITLE);
				pthread_mutex_unlock(g_shared_data.mutex);
				return NULL; // Exit if socket is invalid
			}
		}

		if (g_shared_data.socket_fd > 0)
		{
			printf("\n%sStart activation detection process for buffer %d...\n", PT_TITLE, g_shared_data.buffer_count + 1);

			// Process the buffer
			// mutex is unlocked in processing_pipeline via callback function
			if (processing_pipeline(&g_shared_data.buffer_count, start_idx, &num_activations, activations, unlock_mutex))
			{
				printf("\n%sError occured while processing buffer %d.\n", PT_TITLE, g_shared_data.buffer_count + 1);
				return NULL;
			}
			start_idx += g_buffer_offset;

			printf("%sFinished activation detection process for buffer %d...\n", PT_TITLE, g_shared_data.buffer_count + 1);
			continue; // Continue to next iteration to wait for the next buffer
		}
		else
		{
			// If socket is invalid, exit the thread
			printf("%sSocket connection lost. Exiting process thread...\n", PT_TITLE);
			pthread_mutex_unlock(g_shared_data.mutex);
			break; // Exit if socket is still valid
		}
	}
	printf("%sExiting process thread...\n", PT_TITLE);
	return NULL;
}

void *pacemaker_thread(void *ch_ptr)
{
	// Wait until the socket is valid
	while (1)
	{
		printf("\r%sWaiting for server initialization...", PT_TITLE);
		fflush(stdout); // Print count and clear leftovers
		if (g_shared_data.socket_fd > 0)
		{
			break; // Exit loop if socket is valid
		}
	}

	// print socket_fd
	printf("\n%sServer init confirmed!(socket_fd: %d)\n", PT_TITLE, g_shared_data.socket_fd);
	int ch = *(int *)ch_ptr; // Get the channel number from the argument

	// --- Outer Loop : Client connection loop ---
	while (g_shared_data.socket_fd > 0)
	{
		if (g_shared_data.socket_fd < 1)
		{
			printf("%sSocket connection lost. Exiting pacemaker thread...\n", PT_TITLE);
			if (pthread_mutex_trylock(g_shared_data.mutex) == 0)
			{
				pthread_mutex_unlock(g_shared_data.mutex);
			}
			return NULL; // Exit if socket is invalid
		}

		pthread_mutex_lock(g_shared_data.mutex);
		// reset timer_ms and buffer count
		g_shared_data.buffer_count = 0; // Reset buffer count
		// Wait for a new client connection
		printf("%sWaiting for Gut model connection...\n", PT_TITLE);
		while (g_shared_data.client_fd < 1)
		{
			pthread_cond_wait(g_shared_data.client_connct_cond, g_shared_data.mutex);
		}
		printf("%sGut model connections confirmed!\n", PT_TITLE);
		pthread_mutex_unlock(g_shared_data.mutex);

		// double curr_buff_copy[SIGNAL_PROCESSING_BUFFER_SIZE];
		int start_idx = 0;
		// Processing Loop ---
		printf("%sEntering processing loop...\n", PT_TITLE);

		while (g_shared_data.client_fd > 0)
		{
			pthread_mutex_lock(g_shared_data.mutex);
			while (g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->rtr_flag == 0)
			{
				pthread_cond_wait(g_shared_data.ready_to_read_cond, g_shared_data.mutex);
			}

			// printf("%sBuffer is ready. Processing...\n", PT_TITLE);

			// Check if the socket is still valid
			if (g_shared_data.socket_fd < 1)
			{
				printf("%sSocket connection lost while waiting for buffer to be ready. Exiting pacemaker thread...\n", PT_TITLE);
				pthread_mutex_unlock(g_shared_data.mutex);
				return NULL; // Exit if socket is invalid
			}
			// Check if the client is still connected
			if (g_shared_data.client_fd < 1)
			{
				printf("%sConnection lost while waiting for buffer to be ready. Returning waiting for client connection.\n", PT_TITLE);
				pthread_mutex_unlock(g_shared_data.mutex);
				break; // Exit loop if socket for client is invalid
			}

			// printf("%sBuffer ready. Starting process for buffer %d...\n", PT_TITLE, g_shared_data.buffer_count + 1);

			// Process the buffer
			// mutex is unlocked in processing_pipeline via callback function
			if (run_pacemaker(g_shared_data.pacemaker_data_ptr, g_shared_data.ch_datas_prt[ch], unlock_mutex))
			{
				printf("\n%sError occured while processing buffer %d.\n", PT_TITLE, g_shared_data.buffer_count + 1);
				return NULL;
			}
			start_idx += g_buffer_offset;

			// printf("%sFinished process for buffer %d...\n\n", PT_TITLE, g_shared_data.buffer_count + 1);
		}
	}
	printf("%sExiting process thread...\n", PT_TITLE);
	return NULL;
}
