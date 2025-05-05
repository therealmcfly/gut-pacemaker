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
int tcp_server_accept(void)
{
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
int tcp_server_receive(double *data)
{
	// Receive data from client
	int bytes_read = recv(client_fd, data, sizeof(double), 0);

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
	else if (bytes_read == sizeof(&data))
	{
		return bytes_read;
	}
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

int run_tcp_server(void)
{
	// Set up signal handler for SIGINT
	signal(SIGINT, handle_sigint);

	// Initialize TCP server
	if (tcp_server_init() < 0)
	{
		tcp_server_close();
		printf("\nError: Failed to initialize TCP server.\n");
		return -1;
	}

	// Accept client connection
	if (tcp_server_accept() < 0)
	{
		tcp_server_close();
		printf("\nError: Failed to accept client connection.\n");
		return -1;
	}

	printf("Simulink connected.\n");

	char filename[256] = {0}; // Enough space
	int meta_bytes = recv(client_fd, filename, sizeof(filename) - 1, 0);

	if (meta_bytes > 0)
	{
		printf("\nReceived filename: %s\n", filename);
	}
	else
	{
		printf("\nError: Failed to receive metadata.\n");
	}

	printf("Starting data reception...\n");

	// Buffer for received data
	CircularBufferDouble cir_buffer;
	cb_init(&cir_buffer); // Initialize circular buffer
	double sample;
	bool is_filled = false;

	while (1)
	{
		// Receive data from client
		if (tcp_server_receive(&sample) < 1)
		{
			tcp_server_close();
			printf("\nError: Failed to receive data.\n");
			return -1;
		}
		if (cb_push_sample(&cir_buffer, sample))
		{
			is_filled = true;
			printf("Buffer is full. Starting signal processing...\n");
		}
	}

	tcp_server_close();
	return 0;

	// // Initialize TCP server
	// signal(SIGINT, handle_sigint);
	// struct sockaddr_in address;
	// socklen_t addrlen = sizeof(address);

	// // Create a TCP socket
	// server_fd = socket(AF_INET, SOCK_STREAM, 0);
	// if (server_fd < 0)
	// {
	// 	perror("Socket creation failed");
	// 	exit(EXIT_FAILURE);
	// }

	// // Allow address reuse to avoid 'Address already in use' error
	// int opt = 1;
	// if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	// {
	// 	perror("setsockopt(SO_REUSEADDR) failed");
	// 	close(server_fd);
	// 	exit(EXIT_FAILURE);
	// }

	// // Set up the server addresses stucture
	// address.sin_family = AF_INET;					// IPv4
	// address.sin_addr.s_addr = INADDR_ANY; // Bind to any available address
	// address.sin_port = htons(PORT);				// Convert port number to network byte order

	// // Bind socket to port
	// if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	// {
	// 	perror("Bind failed");
	// 	exit(EXIT_FAILURE);
	// }

	// // Listen for incoming connections
	// if (listen(server_fd, 1) < 0) // 1: max number of pending connections in queue
	// {
	// 	perror("Listen failed");
	// 	exit(EXIT_FAILURE);
	// }
	// CircularBufferDouble cir_buffer;
	// cb_init(&cir_buffer); // Initialize circular buffer

	// printf("Waiting for Simulink to connect on port %d...\n", PORT);

	// // Accept client connection (blocking):
	// client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);

	// // Check if connection failed.
	// if (client_fd < 0)
	// {
	// 	perror("Accept failed");
	// 	exit(EXIT_FAILURE);
	// }

	// printf("Simulink connected.\n");

	// char filename[256] = {0}; // Enough space
	// int meta_bytes = recv(client_fd, filename, sizeof(filename) - 1, 0);

	// if (meta_bytes > 0)
	// {
	// 	printf("Received filename: %s\n", filename);
	// }
	// else
	// {
	// 	printf("Failed to receive metadata.\n");
	// }

	// printf("Starting data reception...\n");

	// double sample;
	// int bytes_read;
	// bool is_filled = false;

	// int count = 1;
	// while (1)
	// {
	// 	bytes_read = recv(client_fd, &sample, sizeof(sample), 0);

	// 	if (bytes_read == 0)
	// 	{
	// 		printf("Connection closed by Simulink.\n");
	// 		break;
	// 	}
	// 	else if (bytes_read < 0)
	// 	{
	// 		perror("recv failed");
	// 		break;
	// 	}
	// 	else if (bytes_read == sizeof(sample))
	// 	{
	// 		// printf("Received %d: %f\n", count, value);

	// 		// Store the received value in the circular buffer

	// 		if (cb_push_sample(&cir_buffer, sample))
	// 		{
	// 			is_filled = true;
	// 			printf("Buffer is full. Starting signal processing...\n");
	// 		}
	// 	}
	// 	count++;
	// }

	// // Close sockets
	// close(client_fd);
	// close(server_fd);
}
