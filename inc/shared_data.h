#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "config.h"
#include <pthread.h>

#include "ring_buffer.h" // If you use RingBufferDouble

#include <stddef.h> // for size_t

typedef struct
{
	// for all threads(Receive and Process)
	RingBuffer *buffer; // pointer (big memory block)
	pthread_mutex_t *mutex;
	pthread_cond_t *client_connct_cond;
	pthread_cond_t *ready_to_read_cond;
	int buffer_count;
	int buff_overlap_count;

	// for Receive Thread
	// int server_fd;
	int client_fd;
	int socket_fd; // for TCP server
								 // for Process Thread

} SharedData;

// Global vars (for static dataset mode, can remove for embedded implimentaion)
extern size_t signal_length;
extern int channel_num;
extern char file_name[100];
extern int cur_data_freq;

extern SharedData shared_data; // Global shared data for all threads

#endif
