#include "networking.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // for close(), usleep()
#include <unistd.h>
#include <arpa/inet.h> // defines sockaddr_in, htons(), INADDR_ANY, and all TCP/IP functions
#include <signal.h>
#include <string.h>

void close_connection(int *fd)
{
	if (*fd > 0)
	{
		close(*fd);
		*fd = -1; // Reset fd to indicate disconnection
		printf("\t❌ Connection closed.\n");
	}
	else
	{
		printf("\n\t⚠️ Unexpected Behaviour: No connection to close.\n");
	}
}
void handle_sigint(int sig)
{
	printf("\n\tSIGINT received. Forcing shutdown.\n");
	_exit(0); // Immediately terminate all threads without cleanup
}
int create_tcp_socket(int *server_fd)
{
	// Create socket
	*server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (*server_fd < 0)
	{
		perror("Error: Socket creation failed");
		return -1;
	}

	// Allow quick reuse of the port after program exits
	int opt = 1;
	if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("Error: setsockopt(SO_REUSEADDR) failed");
		close(*server_fd);
		return -1;
	}

	// Ensure socket closes immediately (no TIME_WAIT delay)
	struct linger sl = {1, 0}; // l_onoff = 1, l_linger = 0
	if (setsockopt(*server_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) < 0)
	{
		perror("Error: setsockopt(SO_LINGER) failed");
		close(*server_fd);
		return -1;
	}
	printf("\tSocket created successfully.\n");
	return 0;
}
int tcp_server_init(int *comm_fd, int port_num)
{
	// Create socket
	if (create_tcp_socket(comm_fd) < 0)
	{
		return -1; // Error already handled in create_tcp_socket
	}

	// Set up server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_num);

	// Bind socket to address
	printf("\tBinding to port %d...\n", port_num);
	if (bind(*comm_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind failed");
		close(*comm_fd);
		return -1;
	}
	printf("\t🟢 Socket bound to port %d successfully.\n", port_num);

	// Listen for incoming connections
	printf("\tAttempting to listen on port %d...\n", port_num);
	if (listen(*comm_fd, 5) < 0)
	{
		perror("\nError: Listen failed");
		close(*comm_fd);
		return -1;
	}

	printf("\t🟢 Server successfully listening on port %d.\n", port_num);
	return *comm_fd;
}
int tcp_server_close(int *gm_fd, int *comm_fd)
{
	if (*gm_fd > 0)
	{
		close(*gm_fd);
		*gm_fd = -1;
	}

	if (*comm_fd > 0)
	{
		close(*comm_fd);
		*comm_fd = -1;
	}
	printf("\n🔴 Server closed.\n");

	return 0;
}
int tcp_server_accept(int *gm_fd, int *comm_fd)
{
	if (*comm_fd < 0)
	{
		fprintf(stderr, "\nError: Server socket is not initialized. Cannot accept client.\n");
		return -1;
	}

	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// Accept incoming connection
	printf("\tListen for client connection...\n");
	*gm_fd = accept(*comm_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (*gm_fd < 0)
	{
		perror("\nError: Client acception failed");

		return -1;
	}
	printf("\t✅ Client connection established: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return *gm_fd;
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
int tcp_server_send(double *data, int size, int *gm_fd)
{
	// Send data to client
	int bytes_sent = send(*gm_fd, data, size, 0);
	if (bytes_sent < 0)
	{
		perror("\nError: Send failed");
		return -1;
	}

	return bytes_sent;
}