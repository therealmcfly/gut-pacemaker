#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "config.h"
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
	RingBuffer *rb;			// pointer to ring buffer
	int ch_num;					// Channel number
	int threshold_flag; // Flag to indicate if threshold is calculated
	int activation_flag;
	int pace_state;
	int lri_ms;
	int gri_ms;
	double lsv_sum; // Sum of lowest slope values
	double threshold;
	int lsv_count;
	int print_interval; // for animation
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
	int *timer_ms;
	// int g_samp_interval_ms;

	// for Receive Thread
	// int server_fd;
	int client_fd;
	int socket_fd; // for TCP server

	// // pacemaker thread
	PacemakerData *p; // Pacemaker data structure

	ChannelData *datas[NUM_CHANNELS];

	// int *timer_ms;
	// int learn_time_ms; // Learning time in milliseconds
	// int gri_thresh_ms;
	// int lri_thresh_ms;
	// int g_samp_interval_ms;

	// int threshold_flag; // Flag to indicate if threshold is calculated
	// int activation_flag;
	// int pace_state;
	// int lri_ms;
	// int gri_ms;
	// double lsv_sum; // Sum of lowest slope values
	// double threshold;
	// int lsv_count;
	// int print_interval; // for animation
} SharedData;

// // Global vars (for static dataset mode, can remove for embedded implimentaion)
// extern size_t signal_length;
// extern int channel_num;
// extern char file_name[100];
// extern int cur_data_freq;
// extern int g_samp_interval_ms; // Sampling interval in milliseconds
// extern int g_buffer_offset;		 // Overlap count for ring buffer

// extern SharedData shared_data; // Global shared data for all threads

#endif
