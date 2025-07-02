#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "config.h"
#include "timer_util.h" // for Timer
#include <pthread.h>

#include "ring_buffer.h" // If you use RingBufferDouble

#include <stddef.h> // for size_t

typedef struct
{
	// pacemaker thread
	int learn_time_ms; // Learning time in milliseconds
	int gri_thresh_ms;
	int lri_thresh_ms;

} PacemakerData;

typedef struct
{
	RingBuffer *ch_rb_ptr; // pointer to ring buffer
	int ch_num;						 // Channel number
	int threshold_flag;		 // Flag to indicate if threshold is calculated
	int activation_flag;
	int pace_state;
	int lri_ms;
	int gri_ms;
	double lsv_sum; // Sum of lowest slope values
	double threshold;
	int lsv_count;
	int print_interval;	 // for animation
	Timer *et_timer_ptr; // Timer for execution time
} ChannelData;

typedef struct
{
	// for all threads(Receive and Process)
	RingBuffer *buffer; // pointer (big memory block)
	pthread_mutex_t *mutex;
	pthread_cond_t *client_connct_cond;
	pthread_cond_t *ready_to_read_cond;
	int buffer_count;
	// int buff_offset;
	int buffer_skipped;
	int *timer_ms_ptr;
	// int g_samp_interval_ms;

	// for Receive Thread
	// int server_fd;
	int client_fd;
	int socket_fd; // for TCP server

	// // pacemaker thread
	PacemakerData *pacemaker_data_ptr; // Pacemaker data structure
	ChannelData *ch_datas_prt[NUM_CHANNELS];

} SharedData;

#endif
