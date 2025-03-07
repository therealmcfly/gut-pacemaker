#ifndef PRE_PROCESSING_H
#define PRE_PROCESSING_H

#include <stddef.h> // for size_t

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

#endif /* SIGNAL_PROCESSING_H */
