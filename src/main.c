#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // for close(), usleep()
#include "config.h"
#include "data_init.h"
#include "signal_buffering.h"
#include "circular_buffer.h"
#include <unistd.h>
#include <arpa/inet.h> // defines sockaddr_in, htons(), INADDR_ANY, and all TCP/IP functions
#include <signal.h>

// TCP Server Constants
#define PORT 8080
#define SAMPLE_DELAY_US 5000 // 200 Hz = 5000 µs delay
#define SAMPLE_COUNT 1000		 // Dummy size; replace with actual size

int server_fd = -1;
int client_fd = -1;

void handle_sigint(int sig)
{
	printf("\nSIGINT received. Closing sockets...\n");
	if (client_fd > 0)
		close(client_fd);
	if (server_fd > 0)
		close(server_fd);
	exit(0);
}

int main(int argc, char *argv[])
{
	// Initialize TCP server
	signal(SIGINT, handle_sigint);
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

	// Create a TCP socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	// Allow address reuse to avoid 'Address already in use' error
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR) failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// Set up the server addresses stucture
	address.sin_family = AF_INET;					// IPv4
	address.sin_addr.s_addr = INADDR_ANY; // Bind to any available address
	address.sin_port = htons(PORT);				// Convert port number to network byte order

	// Bind socket to port
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	// Listen for incoming connections
	if (listen(server_fd, 1) < 0) // 1: max number of pending connections in queue
	{
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}
	CircularBufferDouble cir_buffer;
	cb_init(&cir_buffer); // Initialize circular buffer

	printf("Waiting for Simulink to connect on port %d...\n", PORT);

	// Accept client connection (blocking):
	client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);

	// Check if connection failed.
	if (client_fd < 0)
	{
		perror("Accept failed");
		exit(EXIT_FAILURE);
	}

	printf("Simulink connected.\n");

	char filename[256] = {0}; // Enough space
	int meta_bytes = recv(client_fd, filename, sizeof(filename) - 1, 0);

	if (meta_bytes > 0)
	{
		printf("Received filename: %s\n", filename);
	}
	else
	{
		printf("Failed to receive metadata.\n");
	}

	printf("Starting data reception...\n");

	double sample;
	int bytes_read;
	bool is_filled = false;

	int count = 1;
	while (1)
	{
		bytes_read = recv(client_fd, &sample, sizeof(sample), 0);

		if (bytes_read == 0)
		{
			printf("Connection closed by Simulink.\n");
			break;
		}
		else if (bytes_read < 0)
		{
			perror("recv failed");
			break;
		}
		else if (bytes_read == sizeof(sample))
		{
			// printf("Received %d: %f\n", count, value);

			// Store the received value in the circular buffer

			if (cb_push_sample(&cir_buffer, sample))
			{
				is_filled = true;
				printf("Buffer is full. Starting signal processing...\n");
			}

			// printf("Stored %d: %f\n", count, value);
			// printf("Stored %d: %f\n", count, *cir_buffer.head);
			// printf("Stored %d: %f\n", count, *cir_buffer.tail);
		}
		count++;
	}

	// Close sockets
	close(client_fd);
	close(server_fd);

	// size_t signal_length;
	// int channel_num;
	// char file_name[100]; // Buffer for file name
	// int cur_data_freq;	 // Buffer for exp data frequency
	// // INITIALIZE SAMPLE DATA
	// // Sample data loading, channel selection, and downsampling is all handled within the get_sample_data function
	// // The function will return a pointer to sample data on success, NULL on error
	// double *signal = get_sample_data(argc, argv, &signal_length, &channel_num, file_name, &cur_data_freq);

	// if (signal == NULL)
	// {
	// 	printf("\nError occured while initializing sample data.\n");
	// 	printf("Exiting program...\n\n");
	// 	return 1;
	// }

	// /*----------------------------------------------------------------------------------*/
	// /*----------------------------- SIGNAL BUFFERING -----------------------------------*/
	// /*----------------------------------------------------------------------------------*/

	// if (signal_buffering(signal, signal_length, &channel_num, file_name, &cur_data_freq))
	// {
	// 	printf("\nError occured while buffering signal.\n");
	// 	printf("Exiting program...\n\n");
	// 	return 1;
	// }

	// // Free allocated memory
	// if (signal != NULL)
	// {
	// 	free(signal);
	// 	signal = NULL; // Avoid double free
	// }
	// printf("\nExiting program...\n\n");
	// return 0;
}
