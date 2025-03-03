#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h> // For size_t

#define DATA_DIRECTORY "data/" // Set to data directory from where the executable is being run
#define INITIAL_CAPACITY 1000	 // Start memory allocation for rows
#define DEFAULT_FILE "exp_16_output_512.csv"

// Function to read a specific channel (column) from a CSV file
float *read_data(const char *file_name, int channel_num, size_t *num_rows);
void print_data(float **data, size_t num_rows, size_t num_cols);
int verify_data(float **data, size_t data_num_rows, size_t data_num_cols, const char *file);
float **import_file(const char *file_name, size_t *num_rows, size_t *num_cols);
void get_file_name(char *file_name, size_t file_name_size, int *data_frequency);
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

/**
 * @brief Downsamples the input signal by the specified factor.
 *
 * @param inSignal     Pointer to the input signal array.
 * @param inLength     Number of elements in the input signal array.
 * @param factor       Downsampling factor (e.g. 16).
 * @return             Pointer to the downsampled signal array.
 *                     Returns NULL on error or memory allocation failure.
 */
float *downsample(const float *inSignal,
									size_t *inLength,
									int factor);

/**
 * @brief Retrieve all samples from a single channel (1-based).
 *
 * @param data        Pointer to a 2D array [num_rows][num_cols].
 * @param num_rows    Number of rows in 'data'.
 * @param num_cols    Number of columns in 'data'.
 * @param channel_num 1-based index of the channel to retrieve (1..num_cols).
 * @return            Pointer to newly-allocated array (length = num_rows).
 *                    Returns NULL on error or memory allocation failure.
 */
float *get_ch_signal(float **data,
										 size_t num_rows,
										 size_t num_cols,
										 int channel_num);

#endif
