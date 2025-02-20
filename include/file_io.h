#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h> // For size_t

// Function to read a specific channel (column) from a CSV file
double *read_data(const char *file_name, int channel_num, size_t *num_rows);

#endif
