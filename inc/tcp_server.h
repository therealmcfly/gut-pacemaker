#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "shared_data.h"
#include "timer_util.h"

void handle_sigint(int sig);

int tcp_server_init(int *server_fd);
int tcp_server_accept(int *client_fd, int *server_fd);
int tcp_server_receive(double *data, Timer *interval_timer, int *first_sample, int *client_fd);
int tcp_server_send(double *data, int size, int *client_fd, int *server_fd);
int tcp_server_close(int *client_fd, int *server_fd);

int run_tcp_server(SharedData *shared_data);

#endif