#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <pthread.h>
#include "ring_buffer.h" // If you use RingBufferDouble

typedef struct
{
	// for all threads(Receive and Process)
	RingBuffer *buffer; // pointer (big memory block)
	pthread_mutex_t *mutex;
	pthread_cond_t *ready_to_read_cond;
	int buffer_count;
	int buff_overlap_count;

	// for Receive Thread
	int server_fd;
	int client_fd;
	// for Process Thread

} SharedData;

#endif
