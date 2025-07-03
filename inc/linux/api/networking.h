#ifndef NETWORKING_H
#define NETWORKING_H

#include "shared_data.h"

int create_tcp_socket(int *server_fd);
int tcp_server_init(int *comm_fd, int port_num);
int tcp_server_accept(int *client_fd, int *server_fd);
int tcp_receive(void *data, int data_size, int *fd);
int tcp_server_send(double *data, int size, int *client_fd);
int tcp_server_close(int *client_fd, int *server_fd);
void handle_sigint(int sig);
void close_connection(int *fd);

// int connect_to_server(SharedData *shared_data);

// int run_tcp_server(SharedData *shared_data);
// int run_pacemaker_server(SharedData *shared_data, ChannelData *ch_data);

#endif