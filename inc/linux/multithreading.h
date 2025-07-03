#ifndef MULTITHREADING_H
#define MULTITHREADING_H

void *rd_mode_receive_thread(void *data);
void *gm_tcp_thread(void *ch_ptr);
void *gm_uart_thread(void *ch_ptr);
void *process_thread(void *data);
void *pm_tcp_thread(void *ch_ptr);

#endif // MULTITHREADING_H