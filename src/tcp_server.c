#include "tcp_server.h"

SharedData *g_shared_data = NULL; // Global shared data pointer for SIGINT

// TCP Server Constants
#define PORT 8080
#define SAMPLE_DELAY_US 5000 // 200 Hz = 5000 µs delayactual size

void handle_sigint(int sig)
{
	printf("\nSIGINT received. Closing sockets...\n");
	if (g_shared_data->client_fd > 0)
	{
		printf("Closing client socket...\n");
		close(g_shared_data->client_fd);
		g_shared_data->client_fd = -1;
	}
	if (g_shared_data->server_fd > 0)
	{
		printf("Closing server socket...\n");
		close(g_shared_data->server_fd);
		g_shared_data->server_fd = -1;
	}
	exit(0);
}

int tcp_server_init(int *server_fd)
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

	// Set up server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	// Bind socket to address
	if (bind(*server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind failed");
		close(*server_fd);
		return -1;
	}

	// Listen for incoming connections
	if (listen(*server_fd, 5) < 0)
	{
		perror("\nError: Listen failed");
		close(*server_fd);
		return -1;
	}

	printf("Server listening on port %d...\n", PORT);
	return *server_fd;
}
int tcp_server_close(int *client_fd, int *server_fd)
{
	if (*client_fd > 0)
	{
		close(*client_fd);
		*client_fd = -1;
	}

	if (*server_fd > 0)
	{
		close(*server_fd);
		*server_fd = -1;
	}
	printf("\nServer closed.\n");

	return 0;
}
int tcp_server_accept(int *client_fd, int *server_fd)
{
	printf("Waiting for client connection...\n");
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// Accept incoming connection
	*client_fd = accept(*server_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (*client_fd < 0)
	{
		perror("\nError: Accept failed");

		return -1;
	}

	printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return *client_fd;
}
int tcp_server_send(double *data, int size, int *client_fd, int *server_fd)
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

	// Timing logic here
	if (*first_sample)
	{
		timer_start(interval_timer);
		*first_sample = 0;
	}
	else
	{
		timer_stop(interval_timer);
		double interval_ms = timer_elapsed_ms(interval_timer);
		double freq_hz = interval_ms > 0 ? 1000.0 / interval_ms : 0;
		if (interval_ms > 0)
		{
			// printf("Sample interval: %.3f ms (%.2f Hz)\n", interval_ms, freq_hz);
		}
		else
		{
			printf("Sample interval: %.3f ms (infinity Hz)\n", interval_ms);
		}
		timer_start(interval_timer);
	}

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
	g_shared_data = shared_data; // Set global pointer for signal handler

	signal(SIGINT, handle_sigint);

	if (tcp_server_init(&shared_data->server_fd) < 0)
	{
		printf("\nError: Failed to initialize TCP server.\n");
		return -1;
	}

	while (1) // === Outer loop: wait for new client ===
	{
		if (tcp_server_accept(&shared_data->client_fd, &shared_data->server_fd) < 0)
		{
			printf("\nError: Failed to accept client.\n");
			continue;
		}

		printf("Simulink connected.\n");

		// Receive filename first
		char filename[256] = {0};
		int meta_bytes = recv(shared_data->client_fd, filename, sizeof(filename) - 1, 0);

		if (meta_bytes <= 0)
		{
			printf("Failed to receive filename. Closing connection.\n");
			close(shared_data->client_fd);
			shared_data->client_fd = -1;
			continue; // go back to accept new client
		}

		printf("Received filename: %s\n", filename);
		printf("Starting data reception...\n");

		// Initialize buffer
		rb_init(shared_data->buffer);
		double sample;
		bool initial_full = false;

		// Timer variables
		Timer interval_timer;
		int first_sample = 1;

		printf("Receiving signal...\n");
		while (1) // === Inner loop: receive data from current client ===
		{

			int bytes_read = tcp_server_receive(&sample, &interval_timer, &first_sample, &shared_data->client_fd);

			if (bytes_read == 0)
			{
				printf("Client disconnected.\n");
				break; // exit inner loop, go back to accept()
			}
			else if (bytes_read < 0)
			{
				printf("Receive error. Closing client.\n");
				break;
			}
			else if (bytes_read == sizeof(double))
			{

				rb_push_sample(shared_data->buffer, sample);
				// printf("Received sample: %f\n", sample);

				// before ring buffer is initially filled
				if (!initial_full)
				{
					// check if buffer is full
					if (!shared_data->buffer->is_full)
					{
						shared_data->buffer->ready_to_read = false;
					}
					else
					{
						initial_full = true;
						printf("\nBuffer is full! Ready to process buffer %d.\n", *shared_data->sig_process_count + 1);

						pthread_mutex_lock(shared_data->mutex);
						shared_data->buffer->write_count = 0; // reset write count
						shared_data->buffer->ready_to_read = true;
						pthread_cond_signal(shared_data->cond);
						pthread_mutex_unlock(shared_data->mutex);
					}
					continue; // skip to next iteration
				}

				// after buffer is initially filled
				// check how many signals were writen after last snapshot
				if (!shared_data->buffer->ready_to_read && shared_data->buffer->write_count >= shared_data->buff_overlap_count)
				{
					pthread_mutex_lock(shared_data->mutex);

					printf("\n%d new samples recieved! Ready to process buffer %d\n", shared_data->buffer->write_count, *shared_data->sig_process_count + 1);
					shared_data->buffer->write_count = 0; // reset write count
					shared_data->buffer->ready_to_read = true;
					pthread_cond_signal(shared_data->cond);
					pthread_mutex_unlock(shared_data->mutex);
				}
				// else
				// {
				// 	printf("write_count : %d\n", shared_data->buffer->write_count);
				// }
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

	tcp_server_close(&shared_data->client_fd, &shared_data->server_fd); // Only called if outer loop exits (e.g., via SIGINT)
	return 0;
}
