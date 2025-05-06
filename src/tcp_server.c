#include "tcp_server.h"

int server_fd = -1;
int client_fd = -1;

void handle_sigint(int sig)
{
	printf("\nSIGINT received. Closing sockets...\n");
	if (client_fd > 0)
	{
		printf("Closing client socket...\n");
		close(client_fd);
		client_fd = -1;
	}
	if (server_fd > 0)
	{
		printf("Closing server socket...\n");
		close(server_fd);
		server_fd = -1;
	}
	exit(0);
}

int tcp_server_init(void)
{

	// Create socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		perror("Socket creation failed");
		return -1;
	}

	// Allow quick reuse of the port after program exits
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR) failed");
		close(server_fd);
		return -1;
	}

	// Set up server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	// Bind socket to address
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind failed");
		close(server_fd);
		return -1;
	}

	// Listen for incoming connections
	if (listen(server_fd, 5) < 0)
	{
		perror("\nError: Listen failed");
		close(server_fd);
		return -1;
	}

	printf("Server listening on port %d...\n", PORT);
	return server_fd;
}
int tcp_server_close(void)
{
	if (client_fd > 0)
	{
		close(client_fd);
		client_fd = -1;
	}

	if (server_fd > 0)
	{
		close(server_fd);
		server_fd = -1;
	}
	printf("\nServer closed.\n");

	return 0;
}
int tcp_server_accept(void)
{
	printf("Waiting for client connection...\n");
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// Accept incoming connection
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd < 0)
	{
		perror("\nError: Accept failed");

		return -1;
	}

	printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return client_fd;
}
int tcp_server_send(double *data, int size)
{
	// Send data to client
	int bytes_sent = send(client_fd, data, size * sizeof(double), 0);
	if (bytes_sent < 0)
	{
		perror("\nError: Send failed");
		return -1;
	}

	return bytes_sent;
}
int tcp_server_receive(double *data, Timer *interval_timer, int *first_sample)
{
	// Receive data from client
	int bytes_read = recv(client_fd, data, sizeof(double), 0);

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
		double freq_hz = 1000.0 / interval_ms;
		printf("Sample interval: %.3f ms (%.2f Hz)\n", interval_ms, freq_hz);
		timer_start(interval_timer);
	}

	if (bytes_read == 0)
	{
		printf("\nConnection closed by Simulink.\n");

		return 0;
	}
	else if (bytes_read < 0)
	{
		perror("\nError: recv failed");
		return -1;
	}
	else if (bytes_read != sizeof(double))
	{
		fprintf(stderr, "\nError: recv returned %d bytes, expected %zu bytes\n", bytes_read, sizeof(double));
		return -1;
	}

	return bytes_read;
}

int run_tcp_server(void)
{
	signal(SIGINT, handle_sigint);

	if (tcp_server_init() < 0)
	{
		printf("\nError: Failed to initialize TCP server.\n");
		return -1;
	}

	while (1) // === Outer loop: wait for new client ===
	{
		if (tcp_server_accept() < 0)
		{
			printf("\nError: Failed to accept client.\n");
			continue;
		}

		printf("Simulink connected.\n");

		// Receive filename first
		char filename[256] = {0};
		int meta_bytes = recv(client_fd, filename, sizeof(filename) - 1, 0);

		if (meta_bytes <= 0)
		{
			printf("Failed to receive filename. Closing connection.\n");
			close(client_fd);
			client_fd = -1;
			continue; // go back to accept new client
		}

		printf("Received filename: %s\n", filename);
		printf("Starting data reception...\n");

		// Initialize buffer
		CircularBufferDouble cir_buffer;
		cb_init(&cir_buffer);
		double sample;
		Timer interval_timer;
		int first_sample = 1;

		while (1) // === Inner loop: receive data from current client ===
		{
			int bytes_read = tcp_server_receive(&sample, &interval_timer, &first_sample);

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
				cb_push_sample(&cir_buffer, sample);
				if (cir_buffer.is_full)
				{
					printf("Buffer full. Run detection pipeline...\n");
				}
				else
				{
					printf("Not full. Waiting for more data...\n");
				}
			}
			else
			{
				fprintf(stderr, "Unexpected byte count: %d\n", bytes_read);
			}
		}

		// Clean up after client disconnects
		close(client_fd);
		client_fd = -1;

		// Go back to outer loop to accept a new connection
	}

	tcp_server_close(); // Only called if outer loop exits (e.g., via SIGINT)
	return 0;
}
