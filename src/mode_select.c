#include "mode_select.h"
#include <stdio.h>
#include <pthread.h>
#include "config.h"
#include "data_init.h"
#include "shared_data.h"
#include "multithreading.h"
// #include "signal_buffering.h"
#include "signal_processing.h"

// Initialize global variables
size_t signal_length;
int channel_num;
char file_name[100]; // Buffer for file name
int cur_data_freq;	 // Buffer for exp data frequency

SharedData shared_data;															// Global shared data for all threads
double sp_buffer[SIGNAL_PROCESSING_BUFFER_SIZE];		// Buffer for signal processing
double ad_buffer[ACTIVATION_DETECTION_BUFFER_SIZE]; // Buffer for artifact detection

RunMode select_mode(void)
{
	int choice;
	while (1)
	{
		printf("\nWelcome to Gut Pacemaker!\n");
		printf("\nPlease select a mode:\n");
		printf("\n1. Dataset Mode\n");
		printf("2. Real-time Mode\n");
		printf("3. Gut Model Mode\n");
		printf("4. Test Mode\n");
		printf("\nEnter choice (1-4): ");
		if (scanf("%d", &choice) != 1)
		{
			while (getchar() != '\n')
				; // clear stdin buffer
			printf("Invalid input. Please enter a number.\n");
			continue;
		}
		if (choice >= 1 && choice <= 4)
		{
			while (getchar() != '\n')
			{
				// flush input
			}
			return (RunMode)choice;
		}

		else
			printf("Invalid choice. Try again.\n");
	}
}

int static_dataset_mode(int argc, char *argv[])
{
	// INITIALIZE SAMPLE DATA
	// Sample data loading, channel selection, and downsampling is all handled within the get_sample_data function
	// The function will return a pointer to sample data on success, NULL on error
	double *signal = get_sample_data(argc, argv, &signal_length, &channel_num, file_name, &cur_data_freq);

	if (signal == NULL)
	{
		printf("\nError occured while initializing sample data.\n");

		return 1;
	}

	// Initialize ring buffer
	RingBuffer rb;
	int buffer_size = sizeof(sp_buffer) / sizeof(sp_buffer[0]);
	rb_init(&rb, sp_buffer, buffer_size);
	shared_data.buffer = &rb; // pointer to ring buffer

	/*----------------------------------------------------------------------------------*/
	/*----------------------------- SIGNAL BUFFERING -----------------------------------*/
	/*----------------------------------------------------------------------------------*/

	if (detect_activations(signal, signal_length, &channel_num, file_name, &cur_data_freq))
	{
		printf("\nError occured in static dataset mode.\n");
		if (signal) /* prevent leak on failure */
			free(signal);
		return 1;
	}

	// Free allocated memory
	if (signal != NULL)
	{
		free(signal);
		signal = NULL; // Avoid double free
	}

	return 0;
}

int realtime_dataset_mode(int argc, char *argv[])
{
	// Initialize mutex and condition variable
	pthread_mutex_t buffer_mutex;
	pthread_cond_t client_connct_cond;
	pthread_cond_t ready_to_read_cond;
	pthread_mutex_init(&buffer_mutex, NULL);
	pthread_cond_init(&client_connct_cond, NULL);
	pthread_cond_init(&ready_to_read_cond, NULL);

	// Initialize ring buffer
	RingBuffer rb;
	int buffer_size = sizeof(sp_buffer) / sizeof(sp_buffer[0]);
	rb_init(&rb, sp_buffer, buffer_size);

	// Initialize shared data
	shared_data.buffer = &rb; // pointer to ring buffer
	shared_data.mutex = &buffer_mutex;
	shared_data.client_connct_cond = &client_connct_cond;
	shared_data.ready_to_read_cond = &ready_to_read_cond;
	shared_data.buffer_count = 0;						 // buffer count
	shared_data.buff_offset = BUFFER_OFFSET; // overlap count
	shared_data.socket_fd = -1;							 // server file descriptor
	shared_data.client_fd = -1;							 // client file descriptor

	pthread_t recv_thtread, proc_thread;

	if (pthread_create(&recv_thtread, NULL, rd_mode_receive_thread, NULL) != 0)
	{
		printf("\nError creating TCP server thread.\n");

		return 1;
	}

	if (pthread_create(&proc_thread, NULL, process_thread, NULL) != 0)
	{
		printf("\nError creating signal buffering thread.\n");
		return 1;
	}

	if (pthread_join(recv_thtread, NULL) != 0)
	{
		printf("\nError joining TCP server thread.\n");
		return 1;
	}
	if (pthread_join(proc_thread, NULL) != 0)
	{
		printf("\nError joining signal buffering thread.\n");
		return 1;
	}
	pthread_mutex_destroy(&buffer_mutex);
	pthread_cond_destroy(&ready_to_read_cond);

	return 0;
}

int gut_model_mode(int argc, char *argv[])
{
	// Initialize mutex and condition variable
	pthread_mutex_t buffer_mutex;
	pthread_cond_t client_connct_cond;
	pthread_cond_t ready_to_read_cond;
	pthread_mutex_init(&buffer_mutex, NULL);
	pthread_cond_init(&client_connct_cond, NULL);
	pthread_cond_init(&ready_to_read_cond, NULL);

	// Initialize ring buffer
	RingBuffer ad_rb; // activation detection ring buffer
	int ad_buffer_size = sizeof(ad_buffer) / sizeof(ad_buffer[0]);
	rb_init(&ad_rb, ad_buffer, ad_buffer_size);

	// Initialize shared data
	shared_data.buffer = &ad_rb; // pointer to ring buffer
	shared_data.mutex = &buffer_mutex;
	shared_data.client_connct_cond = &client_connct_cond;
	shared_data.ready_to_read_cond = &ready_to_read_cond;
	shared_data.buffer_count = 0;						 // buffer count
	shared_data.buff_offset = BUFFER_OFFSET; // overlap count
	shared_data.socket_fd = -1;							 // socket file descriptor for TCP server
	// shared_data.server_fd = -1; // server file descriptor
	shared_data.client_fd = -1; // client file descriptor

	// initialize pacemaker thread data
	shared_data.learn_time_ms = LEARN_TIME_MS;
	shared_data.samp_interval_ms = SAMPLING_INTERVAL_MS;
	shared_data.gri_thresh_ms = GRI_THRESHOLD_MS;
	shared_data.lri_thresh_ms = LRI_THRESHOLD_MS;

	int timer_ms = 0;
	shared_data.timer_ms = &timer_ms;
	shared_data.activation_flag = 0;
	shared_data.gri_ms = 0;
	shared_data.lsv_sum = 0.0; // Initialize lowest slope value
	shared_data.lsv_count = 0; // Initialize lowest slope value count
	shared_data.threshold = 0.0;

	pthread_t recv_thtread, proc_thread;

	if (pthread_create(&recv_thtread, NULL, gut_model_mode_receive_thread, NULL) != 0)
	{
		printf("\nError creating TCP server thread.\n");

		return 1;
	}

	if (pthread_create(&proc_thread, NULL, pacemaker_thread, NULL) != 0)
	// if (pthread_create(&proc_thread, NULL, process_thread, NULL) != 0)
	{
		printf("\nError creating signal buffering thread.\n");
		return 1;
	}

	if (pthread_join(recv_thtread, NULL) != 0)
	{
		printf("\nError joining TCP server thread.\n");
		return 1;
	}
	if (pthread_join(proc_thread, NULL) != 0)
	{
		printf("\nError joining signal buffering thread.\n");
		return 1;
	}
	pthread_mutex_destroy(&buffer_mutex);
	pthread_cond_destroy(&ready_to_read_cond);

	// Conn
	return 0;
	// // Initialize mutex and condition variable
	// pthread_mutex_t buffer_mutex;
	// pthread_cond_t client_connct_cond;
	// pthread_cond_t ready_to_read_cond;
	// pthread_mutex_init(&buffer_mutex, NULL);
	// pthread_cond_init(&client_connct_cond, NULL);
	// pthread_cond_init(&ready_to_read_cond, NULL);

	// // Initialize ring buffer
	// rb_init(&cir_buffer, SIGNAL_PROCESSING_BUFFER_SIZE);

	// // Initialize shared data
	// shared_data.buffer = &cir_buffer; // pointer to ring buffer
	// shared_data.mutex = &buffer_mutex;
	// shared_data.client_connct_cond = &client_connct_cond;
	// shared_data.ready_to_read_cond = &ready_to_read_cond;
	// shared_data.buffer_count = 0;						 // buffer count
	// shared_data.buff_offset = BUFFER_OFFSET; // overlap count
	// shared_data.socket_fd = -1;							 // socket file descriptor for TCP server
	// // shared_data.server_fd = -1; // server file descriptor
	// shared_data.client_fd = -1; // client file descriptor

	// pthread_t recv_thtread, proc_thread;

	// if (pthread_create(&recv_thtread, NULL, gut_model_mode_receive_thread, NULL) != 0)
	// {
	// 	printf("\nError creating TCP server thread.\n");

	// 	return 1;
	// }

	// if (pthread_create(&proc_thread, NULL, process_thread, NULL) != 0)
	// // if (pthread_create(&proc_thread, NULL, process_thread, NULL) != 0)
	// {
	// 	printf("\nError creating signal buffering thread.\n");
	// 	return 1;
	// }

	// if (pthread_join(recv_thtread, NULL) != 0)
	// {
	// 	printf("\nError joining TCP server thread.\n");
	// 	return 1;
	// }
	// if (pthread_join(proc_thread, NULL) != 0)
	// {
	// 	printf("\nError joining signal buffering thread.\n");
	// 	return 1;
	// }
	// pthread_mutex_destroy(&buffer_mutex);
	// pthread_cond_destroy(&ready_to_read_cond);

	// // Conn
	// return 0;
}

int test_mode(int argc, char *argv[])
{
	return 1;
}