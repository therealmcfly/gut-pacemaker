#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <pthread.h>
#include "ring_buffer.h" // If you use RingBufferDouble

typedef struct
{
	RingBuffer *buffer;
	pthread_mutex_t *mutex;
	pthread_cond_t *cond;
	int server_fd;
	int client_fd;
} SharedData;

#endif
