#include "multithreading.h"

#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h> // defines sockaddr_in, htons(), INADDR_ANY, and all TCP/IP functions
#include <stdint.h>		 // for close(), usleep()
#include <string.h>
#include "global.h"
#include "signal_processing.h"
#include "networking.h"
#include "ring_buffer.h"
#include "pacemaker.h"
#include "uart_linux.h"

#include "et_log.h"

// TCP Server Constants
#define SAMPLE_DELAY_US 5000 // 200 Hz = 5000 µs delayactual size
#define RECIEVE_SERVER_PORT 8080

// Gut Model Server Constants
#define RD_SERVER_IP "172.23.240.1"
#define RD_SERVER_PORT 8082

// #define RECIEVE_THREAD_PRINT 1 // nucomment to show receive thread print log animation

// Print functions
static void print_received_data(RingBuffer *rb, int ready_buffer_count, int *time_ms)
{
#ifdef RECIEVE_THREAD_PRINT
	printf("\r%s(%d)", RT_TITLE, rb->new_signal_count); // Print count and clear leftovers;
	fflush(stdout);
	if (!(rb->new_signal_count < g_buffer_offset))
	{
		printf(" new samples recieved. Ready to process buffer %d. ", g_shared_data.buffer_count + 1);
		if (!rb->rtr_flag)
		{
			printf("\n");
		}
		else
		{
			printf("(%d buffer skipped)\n", ready_buffer_count);
		}
	}
#endif
	if (time_ms != NULL)
	{
		if (!(rb->new_signal_count < g_buffer_offset))
		{
			if (rb->rtr_flag)
			{
				printf("\n[%.2f](%d skipped)\n", (float)(*time_ms) / 1000.0f, ready_buffer_count);
				g_shared_data.buffer_skipped = 1;
			}
		}
	}
}
static void print_initial_recieved_data(RingBuffer *rb, int is_full)
{
#ifdef RECIEVE_THREAD_PRINT

	if (!is_full)
	{
		printf("\r%s(%d)", RT_TITLE, rb->new_signal_count); // Print count and clear leftovers;
		fflush(stdout);
	}
	else
	{
		printf("\r%s(%d)", RT_TITLE, rb->new_signal_count); // Print count and clear leftovers;
		fflush(stdout);
		printf(" new samples recieved. Ready to process buffer %d. (Buffer filled)\n", g_shared_data.buffer_count + 1);
	}
#endif
}

static void handle_invalid_action()
{
	close_connection(&g_shared_data.comm_fd);
	g_shared_data.comm_fd = -1; // Reset socket fd to indicate disconnection

	// incase the process thread is in waiting state for flags
	pthread_cond_broadcast(g_shared_data.client_connct_cond);
	pthread_cond_broadcast(g_shared_data.ready_to_read_cond);
	// If mutex is locked, unlock it
	if (pthread_mutex_trylock(g_shared_data.mutex) == 0)
	{
		pthread_mutex_unlock(g_shared_data.mutex);
	}
}

void unlock_mutex()
{
	// printf("%sUnlocking mutex...\n", PT_TITLE);
	pthread_mutex_unlock(g_shared_data.mutex);
}

void *rd_mode_receive_thread(void *data)
{
	printf("%sReception thread started...\n", RT_TITLE);

	// connect to matlab server

	signal(SIGINT, handle_sigint);
	struct sockaddr_in server_addr;

	// Create socket
	if (create_tcp_socket(&g_shared_data.comm_fd) < 0)
	{
		printf("\nError: Failed to create TCP socket.\n");
		handle_invalid_action();
		return NULL; // -1
	}
	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(RD_SERVER_PORT);
	if (inet_pton(AF_INET, RD_SERVER_IP, &server_addr.sin_addr) <= 0)
	{
		perror("Invalid address / Not supported");
		close_connection(&g_shared_data.comm_fd);
		handle_invalid_action();
		return NULL; // -1
	}
	// check server address, log the address
	printf("Connecting to gut model server at %s:%d...\n", RD_SERVER_IP, RD_SERVER_PORT);

	if (connect(g_shared_data.comm_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
	{
		printf("Connected to gut model server.\n");
	}
	else
	{
		perror("Connection failed"); // 👈 add this
		close_connection(&g_shared_data.comm_fd);
		handle_invalid_action();
		return NULL; // -1 // 👈 critical!
	}

	pthread_mutex_lock(g_shared_data.mutex);

	// Step 1: Receive mode (1 byte)
	uint8_t mode;
	if (recv(g_shared_data.comm_fd, &mode, sizeof(mode), 0) <= 0)
	{
		perror("Failed to receive mode");
		handle_invalid_action();
		return NULL; // -1
	}
	printf("Received mode: %d\n", mode);

	switch (mode)
	{
	case 1:
		// Step 2: Receive file name
		if (recv(g_shared_data.comm_fd, file_name, sizeof(file_name) - 1, 0) <= 0)
		{
			perror("Failed to receive file name");
			handle_invalid_action();

			return NULL; // -1
		}
		file_name[sizeof(file_name) - 1] = '\0'; // Ensure null-termination
		printf("Received file name: %s\n", file_name);
		// Step 3: Receive frequency
		if (recv(g_shared_data.comm_fd, &cur_data_freq, sizeof(cur_data_freq), 0) != sizeof(cur_data_freq))
		{
			perror("Failed to receive frequency");
			handle_invalid_action();

			return NULL; // -1
		}
		printf("Received frequency: %d Hz\n", cur_data_freq);
		// Step 4: Receive channel number
		if (recv(g_shared_data.comm_fd, &channel_num, sizeof(channel_num), 0) != sizeof(channel_num))
		{
			perror("Failed to receive channel number");

			handle_invalid_action();

			return NULL; // -1
		}
		printf("Received channel number: %d\n", channel_num);

		break;
	case 2:
		printf("Gut Model Mode started.\n");
		break;

	default:
		printf("Unknown mode received: %d\n", mode);
		handle_invalid_action();

		return NULL; // -1 // Invalid mode, exit
	}

	pthread_cond_signal(g_shared_data.client_connct_cond);
	pthread_mutex_unlock(g_shared_data.mutex);
	printf("Recieving gut signal...\n");

	// double sample;
	int init_full_flg = 0;

	// Timer variables
	// Timer interval_timer;
	// int first_sample = 1;

	int ready_buffer_count = 0;

	// recive signal logic
	double gut_signal;
	int cur_time = 0; // Current time in seconds

	while (1)
	{
		if (recv(g_shared_data.comm_fd, &gut_signal, sizeof(double), MSG_WAITALL) <= 0)
		{
			printf("Server disconnected (recv).");
			handle_invalid_action();
			return NULL; // -1 // Exit on error or disconnection
		}
		else
		{
			// printf("Received gut signal: %f\n", gut_signal);
			pthread_mutex_lock(g_shared_data.mutex);
			cur_time = (float)(*(g_shared_data.timer_ms_ptr)) / 1000.0;	 // Convert milliseconds to seconds
			rb_push_sample(g_shared_data.buffer, gut_signal, &cur_time); // this function must only be made when the mutex is locked

			// printf("Sample[%d]: %f\n", g_shared_data.buffer->new_signal_count, sample);

			if (!init_full_flg) // before ring buffer is initially filled
			{
				// check if buffer is full
				if (!g_shared_data.buffer->is_full)
				{
					// Print animation
					printf("\r%s(%d)", RT_TITLE, g_shared_data.buffer->new_signal_count); // Print count and clear leftovers;
					fflush(stdout);
				}
				else
				{
					init_full_flg = 1;
					printf("\r%s(%d)", RT_TITLE, g_shared_data.buffer->new_signal_count); // Print count and clear leftovers;
					fflush(stdout);
					printf(" new samples recieved. Ready to process buffer %d. (Buffer filled)\n", g_shared_data.buffer_count + 1);
					g_shared_data.buffer->rtr_flag = 1;
					g_shared_data.buffer->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(g_shared_data.ready_to_read_cond);
					ready_buffer_count++;
				}

				pthread_mutex_unlock(g_shared_data.mutex);
				continue; // skip to next iteration
			}
			else // after buffer is initially filled
			{
				// check how many signals were writen after last snapshot
				// if (!g_shared_data.buffer->rtr_flag && g_shared_data.buffer->new_signal_count >= g_shared_data.buff_offset)
				if (g_shared_data.buffer->new_signal_count < g_buffer_offset)
				{
					// Print animation
					if (logging_enabled)
					{
						print_received_data(g_shared_data.buffer, ready_buffer_count, NULL);
					}
				}
				else
				{
					(g_shared_data.buffer_count)++;
					// printf("\r%s(%d)", RT_TITLE, g_shared_data.buffer->new_signal_count); // Print count and clear leftovers;
					// fflush(stdout);
					// printf(" new samples recieved. Ready to process buffer %d. ", g_shared_data.buffer_count + 1);
					if (logging_enabled)
					{
						print_received_data(g_shared_data.buffer, ready_buffer_count, NULL);
					}
					if (!g_shared_data.buffer->rtr_flag)
					{
						// printf("\n");
						ready_buffer_count = 0; // reset rtr signal count
						g_shared_data.buffer->rtr_flag = 1;
					}
					// else
					// {
					// 	// printf("(%d buffer skipped)\n", ready_buffer_count);
					// }
					g_shared_data.buffer->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(g_shared_data.ready_to_read_cond);
					ready_buffer_count++;
				}
			}
			pthread_mutex_unlock(g_shared_data.mutex);
		}
	}

	pthread_mutex_lock(g_shared_data.mutex);
	close_connection(&g_shared_data.comm_fd);
	g_shared_data.comm_fd = -1; // Reset socket fd to indicate disconnection
	printf("Disconnect from gut model server.\n");
	// check if the any condition variable is waiting and if so signal them to free the waiting threads
	pthread_cond_broadcast(g_shared_data.client_connct_cond);
	pthread_cond_broadcast(g_shared_data.ready_to_read_cond); // signal ready to read condition variable

	pthread_mutex_unlock(g_shared_data.mutex);

	// if (connect_to_server(&g_shared_data) != 0)
	// {
	// 	printf("\n%sError occured while running TCP server.\n", RT_TITLE);
	// }
	return NULL;
}

void *process_thread(void *data)
{
	pthread_mutex_lock(g_shared_data.mutex);

	printf("%sProcessing thread started. Waiting for client connection...\n", PT_TITLE);
	pthread_cond_wait(g_shared_data.client_connct_cond, g_shared_data.mutex);
	printf("%sClient connected. Starting processing...\n", PT_TITLE);

	if (g_shared_data.comm_fd < 0)
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

	printf("%sSocket fd : %d\n", PT_TITLE, g_shared_data.comm_fd);
	while (g_shared_data.comm_fd > 0)
	{

		pthread_mutex_lock(g_shared_data.mutex);
		while (!g_shared_data.buffer->rtr_flag)
		{
			printf("\n%sWaiting for buffer to be ready...\n", PT_TITLE);
			fflush(stdout);
			pthread_cond_wait(g_shared_data.ready_to_read_cond, g_shared_data.mutex);
			if (g_shared_data.comm_fd > 0)
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

		if (g_shared_data.comm_fd > 0)
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

void *tcp_comm_thread(void *ch_ptr)
{
	int ch = *(int *)ch_ptr; // Get the channel number from the argument
	printf("%sReception thread started...\n", RT_TITLE);

	// run pacemaker server

	// Initialize server
	printf("%sRunning TCP server...\n", RT_TITLE);
	signal(SIGINT, handle_sigint);
	if (tcp_server_init(&g_shared_data.comm_fd, RECIEVE_SERVER_PORT) < 0)
	{
		printf("\nError: Failed to initialize TCP server.\n");
		return NULL; // -1 // -1
	}

	// Start client connect and data reception loop
	printf("%sEntering client connection loop...\n", RT_TITLE);
	while (1)
	// === Outer loop: wait for new client ===
	{
		// Client connection
		if (tcp_server_accept(&g_shared_data.gm_fd, &g_shared_data.comm_fd) < 0)
		{
			printf("\nError: Failed while accepting client connection.\n");
			continue;
		}
		// Client connected, notify other threads
		pthread_mutex_lock(g_shared_data.mutex);
		pthread_cond_signal(g_shared_data.client_connct_cond);
		pthread_mutex_unlock(g_shared_data.mutex);

		// for some reason, ther is a single double sent from the gut model that offsets the buffer. Below code is to receive that double and discard it.

		double initial_recieve = 0.0; // Initialize sample variable
		int bytes = tcp_receive(&initial_recieve, sizeof(double), &g_shared_data.gm_fd);
		if (bytes <= 0)
		{
			printf("Error: Failed to receive initial data from client.\n");
			close_connection(&g_shared_data.gm_fd);
			continue;
		}

		// Initialize variables for receiving data
		double sample;
		int init_full_flg = 0;
		// Timer variables
		// Timer interval_timer;
		// int first_sample = 1;
		// ↓↓↓ var is to show skipped buffers
		int ready_buffer_count = 0;

		double pace_flag_buff; // Initialize pace state
		int cur_time_buff;		 // Current time in seconds

		while (1)
		// === Inner loop: receive data from current client ===
		{
			pthread_mutex_lock(g_shared_data.mutex);
			cur_time_buff = *(g_shared_data.timer_ms_ptr);											// Convert milliseconds to seconds
			pace_flag_buff = (double)g_shared_data.ch_datas_prt[ch]->pace_flag; // Get current pace state
			pthread_mutex_unlock(g_shared_data.mutex);
			if (pace_flag_buff > 0)
			{
				if (!tcp_server_send(&pace_flag_buff, sizeof(double), &g_shared_data.gm_fd))
				{
					printf("Error: Failed to send data to client.\n");
				}
				// printf("[%.2f]Pace state sent: %.2f\n", timer, pace_flag);
			}

			// receive data from client
			int bytes_read = tcp_receive(&sample, sizeof(double), &g_shared_data.gm_fd);

			if (bytes_read != sizeof(double))
			{
				if (bytes_read == 0)
				{
					printf("\n%s🚩 Client disconnected ⛓️‍💥\n", RT_TITLE);
					pthread_mutex_lock(g_shared_data.mutex);									// Close client connection
					rb_reset(g_shared_data.ch_datas_prt[ch]->ch_rb_ptr);			// Reset the ring buffer
					init_full_flg = 0;																				// Reset initial fill flag
					ready_buffer_count = 0;																		// Reset ready buffer count
					*(g_shared_data.timer_ms_ptr) = g_samp_interval_ms;				// Reset timer_ms
					pthread_cond_broadcast(g_shared_data.ready_to_read_cond); // Notify waiting threads
					pthread_mutex_unlock(g_shared_data.mutex);

					break; // exit inner loop, go back to accept()
				}
				else if (bytes_read < 0)
				{
					printf("Receive error. Closing client.\n");
					break;
				}
				else
				{
					fprintf(stderr, "Unexpected byte count: %d\n", bytes_read);
				}
			}
			// printf("\n[%.2f]Received sample: %.10f\n", (float)cur_time_buff / 1000, sample); // Debugging line

			// push received sample to ring buffer
			pthread_mutex_lock(g_shared_data.mutex);

			// ↓↓↓ this function must only be made when the mutex is locked
			rb_push_sample(g_shared_data.ch_datas_prt[0]->ch_rb_ptr, sample, &cur_time_buff);
			// increment timer_ms (by sampling interval)
			*(g_shared_data.timer_ms_ptr) += g_samp_interval_ms;

			if (!init_full_flg) // before ring buffer is initially filled
			{
				// check if buffer is full
				if (!g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->is_full)
				{
					// For printing signal accumulating animation
					print_initial_recieved_data(g_shared_data.ch_datas_prt[ch]->ch_rb_ptr, g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->is_full); // print initial buffer fill
				}
				else
				{
					init_full_flg = 1;
					// For printing signal accumulating animation
					print_initial_recieved_data(g_shared_data.ch_datas_prt[ch]->ch_rb_ptr, g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->is_full); // print initial buffer fill
					//
					g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->rtr_flag = 1;
					g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(g_shared_data.ready_to_read_cond);
					if (logging_enabled)
					{
						timer_start(g_shared_data.ch_datas_prt[ch]->et_timer_ptr); // Start the timer for interval processing
					}
					ready_buffer_count++;
				}

				pthread_mutex_unlock(g_shared_data.mutex);
				continue; // skip to next iteration
			}
			else // after buffer is initially filled
			{
				if (g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->new_signal_count < g_buffer_offset)
				{
					print_received_data(g_shared_data.ch_datas_prt[ch]->ch_rb_ptr, ready_buffer_count, &cur_time_buff);
				}
				else
				{
					(g_shared_data.buffer_count)++;
					print_received_data(g_shared_data.ch_datas_prt[ch]->ch_rb_ptr, ready_buffer_count, &cur_time_buff);
					if (!g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->rtr_flag) // rtr_flag means that the rb_snapshot has occured
					{
						ready_buffer_count = 0;																	 // so reset ready_buffer_count
						g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->rtr_flag = 1; // set rtr_flag to indicate that the buffer is ready to read
					}
					g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(g_shared_data.ready_to_read_cond);
					if (logging_enabled)
					{
						timer_start(g_shared_data.ch_datas_prt[ch]->et_timer_ptr); // Start the timer for interval processing
					}
					ready_buffer_count++;
				}
			}
			pthread_mutex_unlock(g_shared_data.mutex);
		}

		// Clean up after client disconnects
		close_connection(&g_shared_data.gm_fd);

		// Go back to outer loop to accept a new connection
	}

	tcp_server_close(&g_shared_data.gm_fd, &g_shared_data.comm_fd); // Only called if outer loop exits (e.g., via SIGINT)

	// if (run_pacemaker_server(&g_shared_data, g_shared_data.ch_datas_prt[ch]) != 0)
	// {
	// 	printf("\n%sError occured while running pacemaker server.\n", RT_TITLE);
	// }

	return NULL;
}

void *tcp_proc_thread(void *ch_ptr)
{
	// Wait until the socket is valid
	while (1)
	{
		printf("\r%sWaiting for server initialization...", PT_TITLE);
		fflush(stdout); // Print count and clear leftovers
		if (g_shared_data.comm_fd > 0)
		{
			break; // Exit loop if socket is valid
		}
	}

	// print comm_fd
	printf("\n%sServer init confirmed!(comm_fd: %d)\n", PT_TITLE, g_shared_data.comm_fd);
	int ch = *(int *)ch_ptr; // Get the channel number from the argument

	// --- Outer Loop : Client connection loop ---
	while (g_shared_data.comm_fd > 0)
	{
		if (g_shared_data.comm_fd < 1)
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
		while (g_shared_data.gm_fd < 1)
		{
			pthread_cond_wait(g_shared_data.client_connct_cond, g_shared_data.mutex);
		}
		printf("%sGut model connections confirmed!\n", PT_TITLE);
		pthread_mutex_unlock(g_shared_data.mutex);

		// double curr_buff_copy[SIGNAL_PROCESSING_BUFFER_SIZE];
		int start_idx = 0;
		// Processing Loop ---
		printf("%sEntering processing loop...\n", PT_TITLE);

		while (g_shared_data.gm_fd > 0)
		{
			pthread_mutex_lock(g_shared_data.mutex);
			while (g_shared_data.ch_datas_prt[ch]->ch_rb_ptr->rtr_flag == 0)
			{
				pthread_cond_wait(g_shared_data.ready_to_read_cond, g_shared_data.mutex);
			}

			// printf("%sBuffer is ready. Processing...\n", PT_TITLE);

			// Check if the socket is still valid
			if (g_shared_data.comm_fd < 1)
			{
				printf("%sSocket connection lost while waiting for buffer to be ready. Exiting pacemaker thread...\n", PT_TITLE);
				pthread_mutex_unlock(g_shared_data.mutex);
				return NULL; // Exit if socket is invalid
			}
			// Check if the client is still connected
			if (g_shared_data.gm_fd < 1)
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
	return 0;
}

void *uart_comm_thread(void *ch_ptr)
{

	// const char *uart_name = "/dev/virtual_uart"; // for SiL
	const char *uart_name = "/dev/ttyS0"; // for Hil with DE1SoC

	printf("%s▶️ Gut signal thread start!\n", RT_TITLE);

	// Initialize UART device
	signal(SIGINT, handle_sigint); // Handle SIGINT to close UART device gracefully

	printf("%s⌛ Initializing UART device... [%s]\n", RT_TITLE, uart_name);
	g_shared_data.comm_fd = uart_open(uart_name); // or /dev/pts/X for PC test
	if (g_shared_data.comm_fd < 0)
	{
		printf("\n⚠️ Unexpected: Failed to open UART device.\n");
		printf("This might mean [%s] doesnt exist.\n", uart_name);
		printf("Exiting program with status: %d\n\n\n", g_shared_data.comm_fd);
		exit(g_shared_data.comm_fd); // Exit if UART device cannot be opened
		return NULL;
	}
	while (1)
	// === Outer loop: wait for gut model connection ===
	{
		if (g_shared_data.comm_fd < 0)
		{
			printf("\n%s⌛ Reinitializing UART device...\n", RT_TITLE);
			g_shared_data.comm_fd = uart_open(uart_name); // or /dev/pts/X for PC test
			if (g_shared_data.comm_fd < 0)
			{
				printf("\n⚠️ Unexpected: Failed to open UART device.\n");
				printf("This might mean tes [%s] doesnt exist anymore.\n", uart_name);
				printf("Exiting program with status: %d\n\n\n", g_shared_data.comm_fd);
				exit(g_shared_data.comm_fd); // Exit if UART device cannot be opened
				return NULL;
			}
		}

		printf("%s✅ UART device opened!\n", RT_TITLE);

		// variables
		int ch = *(int *)ch_ptr;																	 // Get the channel number from the argument
		ChannelData *ch_data_ptr = g_shared_data.ch_datas_prt[ch]; // Get the channel data pointer

		// Start gut model connection loop
		printf("%sEntering gut model UART connection loop...\n", RT_TITLE);

		// for some reason, ther is a single double sent from the gut model that offsets the buffer. Below code is to receive that double and discard it.
		double dummy;
		// Wait for initial data from gut model
		printf("%s⌛ Waiting for initial data from gut model...\n", RT_TITLE);
		int r = uart_read(&g_shared_data.comm_fd, &dummy, sizeof(double));
		if (r != sizeof(double))
		{
			// print size of data received
			printf("\n❌ Error: Failed to receive initial data from Simulink.\n");
			printf("Exiting program with status: -1\n\n\n");
			exit(-1); // Exit if initial data cannot be received
		}

		pthread_mutex_lock(g_shared_data.mutex);
		g_shared_data.gm_fd = g_shared_data.comm_fd; // Set gm_fd to comm_fd for consistency
		printf("%s✅ UART connection established!(gm_fd: %d)\n", RT_TITLE, g_shared_data.gm_fd);
		pthread_cond_signal(g_shared_data.client_connct_cond);
		pthread_mutex_unlock(g_shared_data.mutex);

		// Initialize variables for receiving data
		double sample;
		int init_full_flg = 0;
		// Timer variables
		// Timer interval_timer;
		// int first_sample = 1;
		// ↓↓↓ var is to show skipped buffers
		int ready_buffer_count = 0;

		int8_t pace_flag;	 // Initialize pace state
		int cur_time_buff; // Current time in seconds

		while (1)
		// === Inner loop: receive data from current client ===
		{
			pthread_mutex_lock(g_shared_data.mutex);

			cur_time_buff = *(g_shared_data.timer_ms_ptr); // Convert milliseconds to seconds

			pace_flag = ch_data_ptr->pace_flag > 0 ? 1 : 0; // Get current pace state
			pthread_mutex_unlock(g_shared_data.mutex);

			int sent = uart_write(&g_shared_data.comm_fd, &pace_flag, sizeof(int8_t));
			if (sent != sizeof(int8_t))
			{
				printf("❌ UART send failed.\n");
			}

			int bytes_read = uart_read(&g_shared_data.comm_fd, &sample, sizeof(double));

			if (bytes_read != sizeof(double))
			{
				printf("\n\n--------------------------------------------------------\n");
				if (bytes_read <= 0)
				{
					printf("\n%s❌ Error or disconnected from gut model ⛓️‍💥(recv bytes: %d)\n", RT_TITLE, bytes_read);
				}
				else
				{
					printf("\n%s❌ Error: Unexpected byte count (recv bytes: %d)\n\n", RT_TITLE, bytes_read);
				}
				pthread_mutex_lock(g_shared_data.mutex);
				rb_reset(ch_data_ptr->ch_rb_ptr);
				init_full_flg = 0;
				ready_buffer_count = 0;
				*(g_shared_data.timer_ms_ptr) = g_samp_interval_ms;
				pthread_cond_broadcast(g_shared_data.ready_to_read_cond);
				pthread_mutex_unlock(g_shared_data.mutex);
				break;
			}
			// printf("\n[%.2f]Received sample: %.10f\n", (float)cur_time_buff / 1000, sample); // Debugging line

			// push received sample to ring buffer
			pthread_mutex_lock(g_shared_data.mutex);

			// ↓↓↓ this function must only be made when the mutex is locked
			rb_push_sample(ch_data_ptr->ch_rb_ptr, sample, &cur_time_buff);
			// increment timer_ms (by sampling interval)
			*(g_shared_data.timer_ms_ptr) += g_samp_interval_ms;

			if (!init_full_flg) // before ring buffer is initially filled
			{
				// check if buffer is full
				if (!ch_data_ptr->ch_rb_ptr->is_full)
				{
					// For printing signal accumulating animation
					print_initial_recieved_data(ch_data_ptr->ch_rb_ptr, ch_data_ptr->ch_rb_ptr->is_full); // print initial buffer fill
				}
				else
				{
					init_full_flg = 1;
					// For printing signal accumulating animation
					print_initial_recieved_data(ch_data_ptr->ch_rb_ptr, ch_data_ptr->ch_rb_ptr->is_full); // print initial buffer fill
					//
					ch_data_ptr->ch_rb_ptr->rtr_flag = 1;
					ch_data_ptr->ch_rb_ptr->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(g_shared_data.ready_to_read_cond);
					if (logging_enabled)
					{
						timer_start(ch_data_ptr->et_timer_ptr); // Start the timer for interval processing
					}
					ready_buffer_count++;
				}

				pthread_mutex_unlock(g_shared_data.mutex);
				continue; // skip to next iteration
			}
			else // after buffer is initially filled
			{
				if (ch_data_ptr->ch_rb_ptr->new_signal_count < g_buffer_offset)
				{
					print_received_data(ch_data_ptr->ch_rb_ptr, ready_buffer_count, &cur_time_buff);
				}
				else
				{
					(g_shared_data.buffer_count)++;
					print_received_data(ch_data_ptr->ch_rb_ptr, ready_buffer_count, &cur_time_buff);
					if (!ch_data_ptr->ch_rb_ptr->rtr_flag) // rtr_flag = 0 means that the rb_snapshot has occured
					{
						ready_buffer_count = 0;								// so reset ready_buffer_count
						ch_data_ptr->ch_rb_ptr->rtr_flag = 1; // set rtr_flag to indicate that the buffer is ready to read
					}
					ch_data_ptr->ch_rb_ptr->new_signal_count = 0; // reset new_signal_count

					pthread_cond_signal(g_shared_data.ready_to_read_cond);
					if (logging_enabled)
					{
						timer_start(ch_data_ptr->et_timer_ptr); // Start the timer for interval processing
					}
					ready_buffer_count++;
				}
			}
			pthread_mutex_unlock(g_shared_data.mutex);
		}

		// Clean up after client disconnects
		g_shared_data.gm_fd = -1; // Reset gm_fd to indicate disconnection
															// Go back to outer loop to accept a new connection
		printf("\n--------------------------------------------------------\n");
		printf("\n%sClosing UART device...\n", RT_TITLE);
		uart_close(&g_shared_data.comm_fd);
	}

	return NULL;
}

void *uart_proc_thread(void *ch_ptr)
{

	printf("%s▶️ Pacemaker thread start!\n", PT_TITLE);
	// Wait until UART is initialized
	if (g_shared_data.comm_fd < 0)
	{
		printf("%s⌛ Waiting for UART device initialization(comm_fd: %d)...\n", PT_TITLE, g_shared_data.comm_fd);
		while (1)
		{
			fflush(stdout); // Print count and clear leftovers
			if (g_shared_data.comm_fd > 0)
			{
				break; // Exit loop if socket is valid
			}
		}
	}

	// print comm_fd
	printf("%s☑️ UART device checked!(comm_fd: %d)\n", PT_TITLE, g_shared_data.comm_fd);
	int ch = *(int *)ch_ptr;																	 // Get the channel number from the argument
	ChannelData *ch_data_ptr = g_shared_data.ch_datas_prt[ch]; // Get the channel data pointer

	// const uint32_t overhead = mt_measure_overhead_us(1000);

	// EtLog L;
	// etlog_init(&L);
	// int et_buffer_full = 0;
	// int et_csv_dumped = 0; // Flag to indicate if ET log has been dumped to CSV

	// --- Outer Loop : Client connection loop ---
	while (g_shared_data.comm_fd >= 0)
	{
		if (g_shared_data.comm_fd < 0)
		{
			printf("\n\n--------------------------------------------------------\n");
			printf("%sUART connection lost. Exiting pacemaker thread...\n", PT_TITLE);
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
		while (g_shared_data.gm_fd < 1)
		{
			printf("%s⌛ Waiting for Gut model connection(gm_fd: %d)...\n", PT_TITLE, g_shared_data.gm_fd);
			pthread_cond_wait(g_shared_data.client_connct_cond, g_shared_data.mutex);
		}
		printf("%s☑️ Gut model connection checked!(gm_fd: %d)\n", PT_TITLE, g_shared_data.gm_fd);
		pthread_mutex_unlock(g_shared_data.mutex);

		// double curr_buff_copy[SIGNAL_PROCESSING_BUFFER_SIZE];
		int start_idx = 0;
		// Processing Loop ---
		printf("%sEntering processing loop...\n", PT_TITLE);

		printf("\n------------------- Pacemaker Logs -------------------\n");

		while (g_shared_data.gm_fd >= 0)
		{
			pthread_mutex_lock(g_shared_data.mutex);

			ch_data_ptr->proc_flag = 0; // set processing to low, indicating processing has ended

			if (ch_data_ptr->mt_ptr->running == 1 && ch_data_ptr->proc_flag == 0)
			{
				et_log_or_dump(ch_data_ptr->mt_ptr, ch_data_ptr->et_log_ptr, ch_data_ptr->timer_overhead, &ch_data_ptr->et_csv_dumped, &ch_data_ptr->et_buffer_full, ch_data_ptr->pm_state, 0); // et log or dump
			}

			while (ch_data_ptr->ch_rb_ptr->rtr_flag == 0)
			{
				pthread_cond_wait(g_shared_data.ready_to_read_cond, g_shared_data.mutex); // wait for RTR signal
			}

			ch_data_ptr->proc_flag = 1;												// set processing to high, indicating processing is ongoing
			mt_start(g_shared_data.ch_datas_prt[ch]->mt_ptr); // Start the timer for interval processing

			// printf("%sBuffer is ready. Processing...\n", PT_TITLE);

			// Check if the socket is still valid
			if (g_shared_data.comm_fd < 0)
			{
				printf("%sSocket connection lost while waiting for buffer to be ready. Exiting pacemaker thread...\n", PT_TITLE);
				pthread_mutex_unlock(g_shared_data.mutex);
				return NULL; // Exit if socket is invalid
			}
			// Check if the client is still connected
			if (g_shared_data.gm_fd < 0)
			{
				printf("%sConnection lost while waiting for buffer to be ready. Returning waiting for client connection.\n", PT_TITLE);
				pthread_mutex_unlock(g_shared_data.mutex);
				break; // Exit loop if socket for client is invalid
			}

			// printf("%sBuffer ready. Starting process for buffer %d...\n", PT_TITLE, g_shared_data.buffer_count + 1);

			// Process the buffer
			// mutex is unlocked in processing_pipeline via callback function
			if (run_pacemaker(g_shared_data.pacemaker_data_ptr, ch_data_ptr, unlock_mutex))
			{
				printf("\n%s❌ Error occured while processing buffer %d.\n", PT_TITLE, g_shared_data.buffer_count + 1);
				return NULL;
			}
			start_idx += g_buffer_offset;

			// printf("%sFinished process for buffer %d...\n\n", PT_TITLE, g_shared_data.buffer_count + 1);
		}
	}
	printf("\n\n--------------------------------------------------------\n");
	printf("%sExiting process thread...\n", PT_TITLE);
	return 0;
}
