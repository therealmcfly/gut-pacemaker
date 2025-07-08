#ifndef UART_LINUX_H
#define UART_LINUX_H

int uart_open(const char *device_path);
int uart_close(int *fd_ptr);
int uart_read(int *fd_ptr, void *buf, int len);
int uart_write(int *fd_ptr, const void *buf, int len);

#endif