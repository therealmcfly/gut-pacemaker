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
#include "signal_processing.h"
#include "ring_buffer.h"
#include "file_io.h"

// SharedData *g_shared_data = NULL; // Global shared data pointer for SIGINT

// TCP Server Constants
#define PORT 8080
#define SAMPLE_DELAY_US 5000 // 200 Hz = 5000 µs delayactual size

// Gut Model Server Constants
#define GUT_SERVER_IP "192.168.32.1"
#define GUT_SERVER_PORT 8082

static void handle_invalid_action(SharedData *shared_data)
{
	close(shared_data->socket_fd);
	shared_data->socket_fd = -1; // Reset socket fd to indicate disconnection
	pthread_cond_broadcast(shared_data->client_connct_cond);
	pthread_cond_broadcast(shared_data->ready_to_read_cond);
	pthread_mutex_unlock(shared_data->mutex);
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

	// // Alternative socket creation method (commented out for now)
	// // Uncomment if you want to use this method instead of create_tcp_socket
	// *server_fd = socket(AF_INET, SOCK_STREAM, 0);
	// if (*server_fd < 0)
	// {
	// 	perror("Socket creation failed");
	// 	return -1;
	// }

	// // Allow quick reuse of the port after program exits
	// int opt = 1;
	// if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	// {
	// 	perror("setsockopt(SO_REUSEADDR) failed");
	// 	close(*server_fd);
	// 	return -1;
	// }

	// // Ensure socket closes immediately (no TIME_WAIT delay)
	// struct linger sl = {1, 0}; // l_onoff = 1, l_linger = 0
	// if (setsockopt(*server_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) < 0)
	// {
	// 	perror("setsockopt(SO_LINGER) failed");
	// 	close(*server_fd);
	// 	return -1;
	// }

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

	printf("Server listening on port %d...\n", PORT);
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
	printf("\nServer closed.\n");

	return 0;
}
int tcp_server_accept(int *client_fd, int *socket_fd)
{
	printf("Waiting for client connection...\n");
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// Accept incoming connection
	*client_fd = accept(*socket_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (*client_fd < 0)
	{
		perror("\nError: Accept failed");

		return -1;
	}

	printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return *client_fd;
}
int tcp_server_send(double *data, int size, int *client_fd)
{
	// Send data to client
	int bytes_sent = send(*client_fd, data, size * sizeof(double), 0);
	if (bytes_sent < 0)
	{
		perror("\nError: Send failed");
		return -1;
	}

	return bytes_sent;
}
int tcp_server_receive(double *data, Timer *interval_timer, int *first_sample, int *client_fd)
{
	// Receive data from client
	int total_read = 0;
	while (total_read < sizeof(double))
	{
		int r = recv(*client_fd, ((char *)data) + total_read, sizeof(double) - total_read, 0);
		if (r <= 0)
			return r; // error or closed
		total_read += r;
	}

	// // Timing logic here
	// if (*first_sample)
	// {
	// 	timer_start(interval_timer);
	// 	*first_sample = 0;
	// }
	// else
	// {
	// 	timer_stop(interval_timer);
	// 	double interval_ms = timer_elapsed_ms(interval_timer);
	// 	double freq_hz = interval_ms > 0 ? 1000.0 / interval_ms : 0;
	// 	if (interval_ms > 0)
	// 	{
	// 		printf("Sample interval: %.3f ms (%.2f Hz)\n", interval_ms, freq_hz);
	// 	}
	// 	else
	// 	{
	// 		printf("Sample interval: %.3f ms (infinity Hz)\n", interval_ms);
	// 	}
	// 	timer_start(interval_timer);
	// }

	if (total_read == 0)
	{
		printf("\nConnection closed by Simulink.\n");

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

	return total_read;
}

int run_tcp_server(SharedData *shared_data)
{
	printf("\n%sRunning TCP server...\n", RT_TITLE);
	signal(SIGINT, handle_sigint);
	if (tcp_server_init(&shared_data->socket_fd) < 0)
	{
		printf("\nError: Failed to initialize TCP server.\n");
		return -1;
	}

	while (1) // === Outer loop: wait for new client ===
	{
		if (tcp_server_accept(&shared_data->client_fd, &shared_data->socket_fd) < 0)
		{
			printf("\nError: Failed to accept client.\n");
			continue;
		}

		pthread_mutex_lock(shared_data->mutex);
		printf("Simulink connected.\n");

		uint8_t mode;

		int mode_bytes = recv(shared_data->client_fd, &mode, sizeof(mode), 0);

		if (mode_bytes <= 0)
		{
			printf("Failed to receive mode. Closing connection.\n");
			close(shared_data->client_fd);
			shared_data->client_fd = -1;
			continue; // go back to accept new client
		}

		// printf("Received mode: %d\n", mode);

		// // Receive filename first

		// int filename_bytes = recv(shared_data->client_fd, file_name, sizeof(file_name) - 1, 0);

		// if (filename_bytes <= 0)
		// {
		// 	printf("Failed to receive filename. Closing connection.\n");
		// 	close(shared_data->client_fd);
		// 	shared_data->client_fd = -1;
		// 	continue; // go back to accept new client
		// }

		// printf("Received filename: %s\n", file_name);

		// int freq_bytes = recv(shared_data->client_fd, &cur_data_freq, sizeof(cur_data_freq), 0);
		// if (freq_bytes != sizeof(cur_data_freq))
		// {
		// 	printf("Failed to receive full int frequency.\n");
		// 	close(shared_data->client_fd);
		// 	shared_data->client_fd = -1;
		// 	continue;
		// }
		// printf("Received frequency information: %d hz\n", cur_data_freq);

		// int channel_bytes = recv(shared_data->client_fd, &channel_num, sizeof(channel_num), 0);

		// if (channel_bytes != sizeof(channel_num))
		// {
		// 	printf("Failed to receive full int channel number.\n");
		// 	close(shared_data->client_fd);
		// 	shared_data->client_fd = -1;
		// 	continue;
		// }

		// printf("Received channel number: %d\n", channel_num);

		pthread_cond_signal(shared_data->client_connct_cond);
		pthread_mutex_unlock(shared_data->mutex);
		printf("Starting data reception...\n");

		double sample;
		int buffer_initial_fill = false;

		// Timer variables
		Timer interval_timer;
		int first_sample = 1;

		int rtr_signal_count = 0;
		while (1) // === Inner loop: receive data from current client ===
		{

			int bytes_read = tcp_server_receive(&sample, &interval_timer, &first_sample, &shared_data->client_fd);

			if (bytes_read == sizeof(double))
			{
				pthread_mutex_lock(shared_data->mutex);

				rb_push_sample(shared_data->buffer, sample); // this function must only be made when the mutex is locked

				// printf("Sample[%d]: %f\n", shared_data->buffer->new_signal_count, sample);

				if (!buffer_initial_fill) // before ring buffer is initially filled
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
						buffer_initial_fill = true;
						printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
						fflush(stdout);
						printf(" new samples recieved. Ready to process buffer %d. (Buffer filled)\n", shared_data->buffer_count + 1);
						shared_data->buffer->rtr_flag = true;
						shared_data->buffer->new_signal_count = 0; // reset new_signal_count
						pthread_cond_signal(shared_data->ready_to_read_cond);
						rtr_signal_count++;
					}

					pthread_mutex_unlock(shared_data->mutex);
					continue; // skip to next iteration
				}
				else // after buffer is initially filled
				{
					// check how many signals were writen after last snapshot
					// if (!shared_data->buffer->rtr_flag && shared_data->buffer->new_signal_count >= shared_data->buff_offset)
					if (shared_data->buffer->new_signal_count < shared_data->buff_offset)
					{
						// Print animation
						printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
						fflush(stdout);
					}
					else
					{
						(shared_data->buffer_count)++;
						printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
						fflush(stdout);
						printf(" new samples recieved. Ready to process buffer %d. ", shared_data->buffer_count + 1);
						shared_data->buffer->new_signal_count = 0; // reset new_signal_count
						if (!shared_data->buffer->rtr_flag)
						{
							printf("\n");
							rtr_signal_count = 0; // reset rtr signal count
							shared_data->buffer->rtr_flag = true;
						}
						else
						{
							printf("(%d buffer skipped)\n", rtr_signal_count);
						}
						pthread_cond_signal(shared_data->ready_to_read_cond);
						rtr_signal_count++;
					}
				}
				pthread_mutex_unlock(shared_data->mutex);
			}
			else if (bytes_read == 0)
			{
				printf("Client disconnected.\n");
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
	server_addr.sin_port = htons(GUT_SERVER_PORT);
	if (inet_pton(AF_INET, GUT_SERVER_IP, &server_addr.sin_addr) <= 0)
	{
		perror("Invalid address / Not supported");
		close(shared_data->socket_fd);
		handle_invalid_action(shared_data);
		return -1;
	}
	// check server address, log the address
	printf("Connecting to gut model server at %s:%d...\n", GUT_SERVER_IP, GUT_SERVER_PORT);

	if (connect(shared_data->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
	{
		printf("Connected to gut model server.\n");
		// return 0; // Successfully connected
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
	int buffer_initial_fill = false;

	// Timer variables
	// Timer interval_timer;
	// int first_sample = 1;

	int rtr_signal_count = 0;

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

			if (!buffer_initial_fill) // before ring buffer is initially filled
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
					buffer_initial_fill = true;
					printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
					fflush(stdout);
					printf(" new samples recieved. Ready to process buffer %d. (Buffer filled)\n", shared_data->buffer_count + 1);
					shared_data->buffer->rtr_flag = true;
					shared_data->buffer->new_signal_count = 0; // reset new_signal_count
					pthread_cond_signal(shared_data->ready_to_read_cond);
					rtr_signal_count++;
				}

				pthread_mutex_unlock(shared_data->mutex);
				continue; // skip to next iteration
			}
			else // after buffer is initially filled
			{
				// check how many signals were writen after last snapshot
				// if (!shared_data->buffer->rtr_flag && shared_data->buffer->new_signal_count >= shared_data->buff_offset)
				if (shared_data->buffer->new_signal_count < shared_data->buff_offset)
				{
					// Print animation
					printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
					fflush(stdout);
				}
				else
				{
					(shared_data->buffer_count)++;
					printf("\r%s(%d)", RT_TITLE, shared_data->buffer->new_signal_count); // Print count and clear leftovers;
					fflush(stdout);
					printf(" new samples recieved. Ready to process buffer %d. ", shared_data->buffer_count + 1);
					shared_data->buffer->new_signal_count = 0; // reset new_signal_count
					if (!shared_data->buffer->rtr_flag)
					{
						printf("\n");
						rtr_signal_count = 0; // reset rtr signal count
						shared_data->buffer->rtr_flag = true;
					}
					else
					{
						printf("(%d buffer skipped)\n", rtr_signal_count);
					}
					pthread_cond_signal(shared_data->ready_to_read_cond);
					rtr_signal_count++;
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