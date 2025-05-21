#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <pthread.h>
#include "ring_buffer.h" // If you use RingBufferDouble

typedef struct
{
	RingBuffer *buffer; // pointer (big memory block)
	pthread_mutex_t *mutex;
	pthread_cond_t *cond;
	int server_fd;
	int client_fd;
	int buff_overlap_count;
	int *sig_process_count;
} SharedData;

#endif
