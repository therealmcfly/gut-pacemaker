#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "config.h"
#include "timer_util.h" // for Timer
#include <pthread.h>

#include "ring_buffer.h" // If you use RingBufferDouble

#include <stddef.h> // for size_t

#include "micro_timer.h" // for MicroTimer
#include <et_log.h>			 // for EtLog
#include <stdatomic.h>

typedef struct
{
	// pacemaker thread
	int learn_time_ms; // Learning time in milliseconds
	int gri_thresh_ms;
	int lri_thresh_ms;

} PacemakerData;

typedef struct
{
	/* Syncronized Data */
	uint8_t pm_state;			 // 0 = learning, 1 = detecting, 2 = Ignore
	RingBuffer *ch_rb_ptr; // pointer to ring buffer
	int ch_num;						 // Channel number
	atomic_int pace_flag;	 // 0 = no pace, 1 = pace requested

	int print_interval;		 // for animation
	Timer *et_timer_ptr;	 // Timer for execution time
	MicroTimer *et_mt_ptr; // MicroTimer for execution time
	uint8_t skip;					 // count of skipped buffers
	uint8_t et_count;

	// et timer vars
	EtLog *et_log_ptr;
	int et_buffer_full;
	int et_csv_dumped;
	uint32_t timer_overhead;
	uint8_t proc_et_flag; // Indicates if processing pipeline ongoing execution

	// Unsynchronized Data
	int threshold_flag; // Flag to indicate if threshold is calculated
	int activation_flag;
	int lri_ms;
	int gri_ms;
	double lsv_sum; // Sum of lowest slope values
	int lsv_count;
	double threshold;
} ChannelData;

typedef struct
{
	// Syncronized Data(for all threads(Comm and Process))
	RingBuffer *buffer; // pointer (big memory block)
	pthread_mutex_t *mutex;
	pthread_cond_t *client_connct_cond;
	pthread_cond_t *ready_to_read_cond;
	int buffer_count;
	// int buff_offset;
	int buffer_skipped;
	int *timer_ms_ptr;
	// int g_samp_interval_ms;

	// Unsynchronized Data(for each thread)
	// Communication thread
	// int server_fd;
	int comm_fd; // for TCP server
	int gm_fd;
	int gut_connct_flag; // Flag to indicate if GUT connection is established

	// Precessing thread
	PacemakerData *pacemaker_data_ptr; // Pacemaker data structure
	ChannelData *ch_datas_prt[NUM_CHANNELS];

} SharedData;

#endif
