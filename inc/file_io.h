#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h> // For size_t

#define DATA_DIRECTORY "data/" // Set to data directory from where the executable is being run
#define INITIAL_CAPACITY 1000	 // Start memory allocation for rows
#define DEFAULT_FILE "exp_16_output.csv"
#define MAX_CHANNEL 2000

// Function to read a specific channel (column) from a CSV file
double *read_data(const char *file_name, int channel_num, size_t *num_rows);
void print_data(double **data, size_t num_rows, size_t num_cols);
int verify_result(double **data, size_t data_num_rows, size_t data_num_cols, const char *file);
double **import_file(const char *file_name, size_t *num_rows, size_t *num_cols);
void get_file_name(char *file_name, size_t file_name_size);
int validate_file_name(const char *file_name);
void get_channel_num(int *channel_num);
int validate_channel_num(int channel_num, int max_channel);

#endif
