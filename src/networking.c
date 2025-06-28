#include "networking.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // for close(), usleep()
#include <unistd.h>
#include <arpa/inet.h> // defines sockaddr_in, htons(), INADDR_ANY, and all TCP/IP functions
#include <signal.h>
#include <pthread.h>

#include "config.h"
#include "global.h"
#include "signal_processing.h"
#include "ring_buffer.h"
#include "file_io.h"
// #include "shared_data.h"

// SharedData *g_shared_data = NULL; // Global shared data pointer for SIGINT

// TCP Server Constants
#define PORT 8080
#define SAMPLE_DELAY_US 5000 // 200 Hz = 5000 µs delayactual size

// Gut Model Server Constants
#define RD_SERVER_IP "172.23.240.1"
#define RD_SERVER_PORT 8082

// #define RECIEVE_THREAD_PRINT 1 // nucomment to show receive thread print log animation

// Print functions
static void print_recieved_data(RingBuffer *rb, int ready_buffer_count)
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
// Internal utility functions
static void close_client(int *client_fd)
{
	if (*client_fd > 0)
	{
		close(*client_fd);
		*client_fd = -1; // Reset client fd to indicate disconnection
		printf("Client connection closed.\n");
	}
	else
	{
		printf("Unexpected Behaviour: No client connection to close.\n");
	}
}
static void handle_invalid_action(SharedData *shared_data)
{
	close(shared_data->socket_fd);
	shared_data->socket_fd = -1; // Reset socket fd to indicate disconnection

	// incase the process thread is in waiting state for flags
	pthread_cond_broadcast(shared_data->client_connct_cond);
	pthread_cond_broadcast(shared_data->ready_to_read_cond);
	// If mutex is locked, unlock it
	if (pthread_mutex_trylock(shared_data->mutex) == 0)
	{
		pthread_mutex_unlock(shared_data->mutex);
	}
}
static void handle_sigint(int sig)
{
	printf("\nSIGINT received. Forcing shutdown.\n");
	_exit(0); // Immediately terminate all threads without cleanup
}
static int create_tcp_socket(int *server_fd)
{
	// Create socket
	*server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (*server_fd < 0)
	{
		perror("Socket creation failed");
		return -1;
	}

	// Allow quick reuse of the port after program exits
	int opt = 1;
	if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR) failed");
		close(*server_fd);
		return -1;
	}

	// Ensure socket closes immediately (no TIME_WAIT delay)
	struct linger sl = {1, 0}; // l_onoff = 1, l_linger = 0
	if (setsockopt(*server_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) < 0)
	{
		perror("setsockopt(SO_LINGER) failed");
		close(*server_fd);
		return -1;
	}
	printf("Socket created successfully.\n");
	return 0;
}
int tcp_server_init(int *socket_fd)
{
	// Create socket
	if (create_tcp_socket(socket_fd) < 0)
	{
		return -1; // Error already handled in create_tcp_socket
	}

	// Set up server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	// Bind socket to address
	if (bind(*socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind failed");
		close(*socket_fd);
		return -1;
	}

	// Listen for incoming connections
	if (listen(*socket_fd, 5) < 0)
	{
		perror("\nError: Listen failed");
		close(*socket_fd);
		return -1;
	}

	printf("🟢 Server listening on port %d...\n", PORT);
	return *socket_fd;
}
int tcp_server_close(int *client_fd, int *socket_fd)
{
	if (*client_fd > 0)
	{
		close(*client_fd);
		*client_fd = -1;
	}

	if (*socket_fd > 0)
	{
		close(*socket_fd);
		*socket_fd = -1;
	}
	printf("\n🔴 Server closed.\n");

	return 0;
}
int tcp_server_accept(int *client_fd, int *socket_fd)
{
	if (*socket_fd < 0)
	{
		fprintf(stderr, "\nError: Server socket is not initialized. Cannot accept client.\n");
		return -1;
	}

	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// Accept incoming connection
	*client_fd = accept(*socket_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (*client_fd < 0)
	{
		perror("\nError: Client acception failed");

		return -1;
	}

	printf("✅ Client connect accepted: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return *client_fd;
}
int tcp_receive(void *data, int data_size, int *fd)
{
	// Receive data from server or client
	int total_read = 0;
	uint8_t *p = (uint8_t *)data;
	while (total_read < data_size)
	{
		int r = recv(*fd, p + total_read, data_size - total_read, 0);
		if (r <= 0)
			return r; // error or closed
		total_read += r;
	}

	if (total_read == 0)
	{
		printf("\nConnection closed by client.\n");

		return 0;
	}
	else if (total_read < 0)
	{
		perror("\nError: recv failed");
		return -1;
	}
	else if (total_read != sizeof(double))
	{
		fprintf(stderr, "\nError: recv returned %d bytes, expected %zu bytes\n", total_read, sizeof(double));
		return -1;
	}
	else
	{
		// printf("\nReceived %d bytes from client.\n", total_read);
		return total_read;
	}
}
int tcp_server_send(double *data, int size, int *client_fd)
{
	// Send data to client
	int bytes_sent = send(*client_fd, data, size, 0);
	if (bytes_sent < 0)
	{
		perror("\nError: Send failed");
		return -1;
	}

	return bytes_sent;
}
int run_pacemaker_server(SharedData *shared_data, ChannelData *ch_data)
{
	// Initialize server
	printf("%sRunning TCP server...\n", RT_TITLE);
	signal(SIGINT, handle_sigint);
	if (tcp_server_init(&shared_data->socket_fd) < 0)
	{
		printf("\nError: Failed to initialize TCP server.\n");
		return -1;
	}

	// Start client connect and data reception loop
	while (1)
	// === Outer loop: wait for new client ===
	{
		// Client connection
		if (tcp_server_accept(&shared_data->client_fd, &shared_data->socket_fd) < 0)
		{
			printf("\nError: Failed while accepting client connection.\n");
			continue;
		}
		// Client connected, notify other threads
		pthread_mutex_lock(shared_data->mutex);
		printf("Gut model connect accepted.\n");
		pthread_cond_signal(shared_data->client_connct_cond);
		pthread_mutex_unlock(shared_data->mutex);

		// for some reason, ther is a single double sent from the gut model that offsets the buffer. Below code is to receive that double and discard it.

		double initial_recieve = 0.0; // Initialize sample variable
		int bytes = tcp_receive(&initial_recieve, sizeof(double), &shared_data->client_fd);

		printf("Starting data reception...\n");

		// Initialize variables for receiving data
		double sample;
		int init_full_flg = 0;
		// Timer variables
		// Timer interval_timer;
		// int first_sample = 1;
		// ↓↓↓ var is to show skipped buffers
		int ready_buffer_count = 0;

		while (1)
		// === Inner loop: receive data from current client ===
		{
			pthread_mutex_lock(shared_data->mutex);
			float timer = (float)(*(shared_data->timer_ms_ptr)) / 1000.0; // Convert milliseconds to seconds
			double pace_state = (double)ch_data->pace_state;							// Get current pace state
			pthread_mutex_unlock(shared_data->mutex);
			if (pace_state > 0)
			{
				if (!tcp_server_send(&pace_state, sizeof(double), &shared_data->client_fd))
				{
					printf("Error: Failed to send data to client.\n");
				}
				// printf("[%.2f]Pace state sent: %.2f\n", timer, pace_state);
			}

			// receive data from client
			int bytes_read = tcp_receive(&sample, sizeof(double), &shared_data->client_fd);

			if (bytes_read != sizeof(double))
			{
				if (bytes_read == 0)
				{
					printf("🔴 Client disconnected.\n");
					pthread_mutex_lock(shared_data->mutex);
					close_client(&shared_data->client_fd);									 // Close client connection
					rb_reset(ch_data->ch_rb_ptr);														 // Reset the ring buffer
					init_full_flg = 0;																			 // Reset initial fill flag
					ready_buffer_count = 0;																	 // Reset ready buffer count
					pthread_cond_broadcast(shared_data->ready_to_read_cond); // Notify waiting threads
					pthread_mutex_unlock(shared_data->mutex);

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
			// printf("[%.2f]Received sample: %.10f\n", timer, sample); // Debugging line

			// push received sample to ring buffer
			pthread_mutex_lock(shared_data->mutex);

			// ↓↓↓ this function must only be made when the mutex is locked
			rb_push_sample(shared_data->ch_datas_prt[0]->ch_rb_ptr, sample);
			// increment timer_ms (by sampling interval)
			*(shared_data->timer_ms_ptr) += g_samp_interval_ms;

			if (!init_full_flg) // before ring buffer is initially filled
			{
				// check if buffer is full
				if (!ch_data->ch_rb_ptr->is_full)
				{
					// For printing signal accumulating animation
					print_initial_recieved_data(ch_data->ch_rb_ptr, ch_data->ch_rb_ptr->is_full); // print initial buffer fill
				}
				else
				{
					init_full_flg = 1;
					// For printing signal accumulating animation
					print_initial_recieved_data(ch_data->ch_rb_ptr, ch_data->ch_rb_ptr->is_full); // print initial buffer fill
					//
					ch_data->ch_rb_ptr->rtr_flag = 1;
					ch_data->ch_rb_ptr->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(shared_data->ready_to_read_cond);
					ready_buffer_count++;
				}

				pthread_mutex_unlock(shared_data->mutex);
				continue; // skip to next iteration
			}
			else // after buffer is initially filled
			{
				if (ch_data->ch_rb_ptr->new_signal_count < g_buffer_offset)
				{
					print_recieved_data(ch_data->ch_rb_ptr, ready_buffer_count);
				}
				else
				{
					(shared_data->buffer_count)++;
					print_recieved_data(ch_data->ch_rb_ptr, ready_buffer_count);
					if (!ch_data->ch_rb_ptr->rtr_flag)
					{
						ready_buffer_count = 0; // rtr_flag being false indicates that the buffer snapshot occured so reset ready_buffer_count
						ch_data->ch_rb_ptr->rtr_flag = 1;
					}
					ch_data->ch_rb_ptr->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(shared_data->ready_to_read_cond);
					ready_buffer_count++;
				}
			}
			pthread_mutex_unlock(shared_data->mutex);
		}

		// Clean up after client disconnects
		close(shared_data->client_fd);
		shared_data->client_fd = -1;

		// Go back to outer loop to accept a new connection
	}

	tcp_server_close(&shared_data->client_fd, &shared_data->socket_fd); // Only called if outer loop exits (e.g., via SIGINT)
	return 0;
}

int connect_to_server(SharedData *shared_data)
{
	signal(SIGINT, handle_sigint);
	struct sockaddr_in server_addr;

	// Create socket
	if (create_tcp_socket(&shared_data->socket_fd) < 0)
	{
		printf("\nError: Failed to create TCP socket.\n");
		handle_invalid_action(shared_data);
		return -1;
	}
	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(RD_SERVER_PORT);
	if (inet_pton(AF_INET, RD_SERVER_IP, &server_addr.sin_addr) <= 0)
	{
		perror("Invalid address / Not supported");
		close(shared_data->socket_fd);
		handle_invalid_action(shared_data);
		return -1;
	}
	// check server address, log the address
	printf("Connecting to gut model server at %s:%d...\n", RD_SERVER_IP, RD_SERVER_PORT);

	if (connect(shared_data->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
	{
		printf("Connected to gut model server.\n");
	}
	else
	{
		perror("Connection failed"); // 👈 add this
		close(shared_data->socket_fd);
		handle_invalid_action(shared_data);
		return -1; // 👈 critical!
	}

	pthread_mutex_lock(shared_data->mutex);

	// Step 1: Receive mode (1 byte)
	uint8_t mode;
	if (recv(shared_data->socket_fd, &mode, sizeof(mode), 0) <= 0)
	{
		perror("Failed to receive mode");
		handle_invalid_action(shared_data);
		return -1;
	}
	printf("Received mode: %d\n", mode);

	switch (mode)
	{
	case 1:
		// Step 2: Receive file name
		if (recv(shared_data->socket_fd, file_name, sizeof(file_name) - 1, 0) <= 0)
		{
			perror("Failed to receive file name");
			handle_invalid_action(shared_data);

			return -1;
		}
		file_name[sizeof(file_name) - 1] = '\0'; // Ensure null-termination
		printf("Received file name: %s\n", file_name);
		// Step 3: Receive frequency
		if (recv(shared_data->socket_fd, &cur_data_freq, sizeof(cur_data_freq), 0) != sizeof(cur_data_freq))
		{
			perror("Failed to receive frequency");
			handle_invalid_action(shared_data);

			return -1;
		}
		printf("Received frequency: %d Hz\n", cur_data_freq);
		// Step 4: Receive channel number
		if (recv(shared_data->socket_fd, &channel_num, sizeof(channel_num), 0) != sizeof(channel_num))
		{
			perror("Failed to receive channel number");

			handle_invalid_action(shared_data);

			return -1;
		}
		printf("Received channel number: %d\n", channel_num);

		break;
	case 2:
		printf("Gut Model Mode started.\n");
		break;

	default:
		printf("Unknown mode received: %d\n", mode);
		handle_invalid_action(shared_data);

		return -1; // Invalid mode, exit
	}

	pthread_cond_signal(shared_data->client_connct_cond);
	pthread_mutex_unlock(shared_data->mutex);
	printf("Recieving gut signal...\n");

	// double sample;
	int init_full_flg = 0;

	// Timer variables
	// Timer interval_timer;
	// int first_sample = 1;

	int ready_buffer_count = 0;

	// recive signal logic
	double gut_signal;

	while (1)
	{
		if (recv(shared_data->socket_fd, &gut_signal, sizeof(double), MSG_WAITALL) <= 0)
		{
			printf("Server disconnected (recv).");
			handle_invalid_action(shared_data);
			return -1; // Exit on error or disconnection
		}
		else
		{
			// printf("Received gut signal: %f\n", gut_signal);
			pthread_mutex_lock(shared_data->mutex);
			rb_push_sample(shared_data->buffer, gut_signal); // this function must only be made when the mutex is locked

			// printf("Sample[%d]: %f\n", shared_data->buffer->new_signal_count, sample);

			if (!init_full_flg) // before ring buffer is initially filled
			{
				// check if buffer is full
				if (!shared_data->buffer->is_full)
				{
					// Print animation
					printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
					fflush(stdout);
				}
				else
				{
					init_full_flg = 1;
					printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
					fflush(stdout);
					printf(" new samples recieved. Ready to process buffer %d. (Buffer filled)\n", shared_data->buffer_count + 1);
					shared_data->buffer->rtr_flag = 1;
					shared_data->buffer->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(shared_data->ready_to_read_cond);
					ready_buffer_count++;
				}

				pthread_mutex_unlock(shared_data->mutex);
				continue; // skip to next iteration
			}
			else // after buffer is initially filled
			{
				// check how many signals were writen after last snapshot
				// if (!shared_data->buffer->rtr_flag && shared_data->buffer->new_signal_count >= shared_data->buff_offset)
				if (shared_data->buffer->new_signal_count < g_buffer_offset)
				{
					// Print animation
					print_recieved_data(shared_data->buffer, ready_buffer_count);
				}
				else
				{
					(shared_data->buffer_count)++;
					// printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
					// fflush(stdout);
					// printf(" new samples recieved. Ready to process buffer %d. ", shared_data->buffer_count + 1);
					print_recieved_data(shared_data->buffer, ready_buffer_count);
					if (!shared_data->buffer->rtr_flag)
					{
						// printf("\n");
						ready_buffer_count = 0; // reset rtr signal count
						shared_data->buffer->rtr_flag = 1;
					}
					// else
					// {
					// 	// printf("(%d buffer skipped)\n", ready_buffer_count);
					// }
					shared_data->buffer->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(shared_data->ready_to_read_cond);
					ready_buffer_count++;
				}
			}
			pthread_mutex_unlock(shared_data->mutex);
		}
	}

	// //send pacing logic
	// sleep(5); // Wait for 5 seconds before starting pacing
	// int count = 1;
	// uint8_t pace = 1;
	// int sock = shared_data->socket_fd; // Use the socket from shared_data
	// while (1)
	// {
	// 	if (send(sock, &pace, sizeof(uint8_t), 0) != sizeof(uint8_t))
	// 	{
	// 		perror("Pacing send failed");
	// 		break;
	// 	}
	// 	printf("Pacing %d\n", count++);
	// 	sleep(5);
	// }

	// 	{
	// 		printf("Connected to gut model server.\n");
	// 		return 0;
	// 	}

	// // 3. Connect to the gut model server
	// int max_attempts = 5;
	// int attempts = 0;

	// while (attempts < max_attempts)
	// {
	// 	if (connect(shared_data->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
	// 	{
	// 		printf("Connected to gut model server.\n");
	// 		return 0;
	// 	}

	// 	perror("Connection attempt failed");
	// 	attempts++;
	// 	sleep(1); // wait before retrying
	// }

	// if (attempts == max_attempts)
	// {
	// 	printf("Failed to connect to gut model server after %d attempts.\n", max_attempts);
	// 	close(shared_data->socket_fd);
	// 	return -1;
	// }

	pthread_mutex_lock(shared_data->mutex);

	close(shared_data->socket_fd);
	shared_data->socket_fd = -1; // Reset socket fd to indicate disconnection
	printf("Disconnect from gut model server.\n");
	// check if the any condition variable is waiting and if so signal them to free the waiting threads
	pthread_cond_broadcast(shared_data->client_connct_cond);
	pthread_cond_broadcast(shared_data->ready_to_read_cond); // signal ready to read condition variable

	pthread_mutex_unlock(shared_data->mutex);

	return 0;
}

int run_tcp_server(SharedData *shared_data)
{

	signal(SIGINT, handle_sigint);
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// Create socket
	if ((shared_data->socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket failed");
		return 1;
	}

	// Bind
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
	server_addr.sin_port = htons(PORT);

	if (bind(shared_data->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind failed");
		return 1;
	}

	// Listen
	if (listen(shared_data->socket_fd, 1) < 0)
	{
		perror("listen failed");
		return 1;
	}

	printf("🟢 Waiting for connection on port %d...\n", PORT);

	// Accept client
	if ((shared_data->client_fd = accept(shared_data->socket_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0)
	{
		perror("accept failed");
		return 1;
	}

	pthread_mutex_lock(shared_data->mutex);
	printf("✅ Client connected.\n");

	// Receive loop
	while (1)
	{
		double val;
		int total = 0;
		uint8_t *p = (uint8_t *)&val;

		// Receive 8 bytes (one double)
		while (total < sizeof(double))
		{
			int r = recv(shared_data->client_fd, p + total, sizeof(double) - total, 0);
			if (r <= 0)
			{
				printf("🔴 Connection closed or error.\n");
				close(shared_data->client_fd);
				close(shared_data->socket_fd);
				return 0;
			}
			total += r;
		}

		// Print raw bytes
		printf("Bytes: ");
		for (int i = 0; i < 8; ++i)
			printf("%02x ", p[i]);
		printf("=> Value: %.15f\n", val);
	}

	return 0;
}