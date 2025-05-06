#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // for close(), usleep()
#include <unistd.h>
#include <arpa/inet.h> // defines sockaddr_in, htons(), INADDR_ANY, and all TCP/IP functions
#include <signal.h>

#include "config.h"
#include "signal_buffering.h"
#include "circular_buffer.h"

// TCP Server Constants
#define PORT 8080
#define SAMPLE_DELAY_US 5000 // 200 Hz = 5000 µs delayactual size

void handle_sigint(int sig);

int tcp_server_init(void);
int tcp_server_accept(void);
int tcp_server_receive(double *data);
int tcp_server_send(double *data, int size);
int tcp_server_close();

int run_tcp_server(void);

#endif // MODE_SELECT_H