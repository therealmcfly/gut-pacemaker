#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h> // For size_t

// Function to read a specific channel (column) from a CSV file
double *read_data(const char *file_name, int channel_num, size_t *num_rows);
void print_data(double **data, size_t num_rows, size_t num_cols);

double **import_file(const char *file_name, size_t *num_rows, size_t *num_cols);
void get_file_name(char *file_name, int *data_frequency);
/**
 * @brief Validate file name structure and extract frequency from the name.
 *
 * Expected structure: <any_name>_<frequency>.ext
 *   - The frequency part must be numeric.
 *   - The extension must be one of ".csv" or ".bin".
 *
 * @param file_name     Name of the file (e.g. "exp_16_output_512.csv").
 * @param out_frequency Pointer to int, where extracted frequency is stored on success.
 * @return              0 on success, 1 on error.
 */
int validate_file_name(const char *file_name, int *out_frequency);
void get_channel_num(int *channel_num, int max_channel);
int validate_channel_num(int channel_num, int max_channel);
int verify_data(double **data, size_t data_num_rows, size_t data_num_cols, const char *file);

#endif
