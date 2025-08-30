#ifndef MULTITHREADING_H
#define MULTITHREADING_H

void *rd_mode_receive_thread(void *data);
void *process_thread(void *data);
void *tcp_comm_thread(void *ch_ptr);
void *tcp_proc_thread(void *ch_ptr);
void *uart_comm_thread(void *ch_ptr);
void *uart_proc_thread(void *ch_ptr);

#endif // MULTITHREADING_H