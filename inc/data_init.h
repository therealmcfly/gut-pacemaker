#ifndef DATA_INIT_H
#define DATA_INIT_H

#include <stddef.h> // for size_t

double *get_sample_data(int user_argc, char *user_argv[], size_t *out_data_length, int *out_channel_num);
/**
 * @brief Downsamples the input signal by the specified factor.
 *
 * @param inSignal     Pointer to the input signal array.
 * @param inLength     Number of elements in the input signal array.
 * @param factor       Downsampling factor (e.g. 16).
 * @return             Pointer to the downsampled signal array.
 *                     Returns NULL on error or memory allocation failure.
 */
double *downsample(const double *inSignal,
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
double *get_ch_signal(double **data,
											size_t num_rows,
											size_t num_cols,
											int channel_num);

/**
 * @brief Verifies signal data against reference data.
 *
 * @param signal        Pointer to the signal data to verify.
 * @param num_of_data   Number of elements in the signal array.
 * @param verify_data   Pointer to the verification data array.
 * @param verify_num_rows Number of rows in the verification data.
 * @param verify_num_cols Number of columns in the verification data.
 * @return              0 on success, non-zero on error.
 */
int verify_signals(double *signal, size_t num_of_data, double **verify_data, size_t *verify_num_rows, size_t *verify_num_cols);

#endif
