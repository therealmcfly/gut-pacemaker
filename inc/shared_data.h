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
	int buff_offset;

	// for Receive Thread
	// int server_fd;
	int client_fd;
	int socket_fd; // for TCP server

	// pacemaker thread
	int *timer_ms;
	int learn_time_ms;	// Learning time in milliseconds
	int threshold_flag; // Flag to indicate if threshold is calculated
	int activation_flag;
	int pace_flag;
	int lri_ms;
	int gri_ms;
	double lsv_sum; // Sum of lowest slope values
	double threshold;
	int samp_interval_ms;
	int lsv_count;
	int gri_thresh_ms;
	int lri_thresh_ms;
	int print_interval; // for animation
} SharedData;

typedef struct
{
	// Threshold for pacing
} PacemakerData;

// Global vars (for static dataset mode, can remove for embedded implimentaion)
extern size_t signal_length;
extern int channel_num;
extern char file_name[100];
extern int cur_data_freq;

extern SharedData shared_data; // Global shared data for all threads

#endif
