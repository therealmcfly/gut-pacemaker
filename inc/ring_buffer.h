#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <config.h>

// Circular buffer size

typedef struct
{
	double buffer[BUFFER_SIZE]; // Fixed-size buffer
	int size;
	double *head;					// Write pointer
	double *tail;					// Read pointer
	double *end;					// Pointer to buffer + BUFFER_SIZE
	bool is_full;					// Flag to indicate if the buffer is full
	bool rtr_flag;				// Flag to indicate if the buffer is ready
	int new_signal_count; // count of write after last read
} RingBuffer;

/**
 + * Initializes a ring buffer
 + * @param cb Pointer to the RingBuffer to initialize
 + */
void rb_init(RingBuffer *cb);

/**
 * Pushes a sample into the ring buffer
 *
 * @param cb Pointer to the RingBuffer to push data into
 * @param data The sample data to push
 * @return true if the sample was successfully pushed, false otherwise
 */
bool rb_push_sample(RingBuffer *cb, double data);

/**
 * Takes a snapshot of the ring buffer
 *
 * @param rb Pointer to the RingBuffer to snapshot
 * @param buff_copy Pointer to the buffer to copy data into
 * @param next_overlap_count Number of samples to overlap in the next buffer
 * @param reset_flag Function pointer to reset the flag
 * @return true if the snapshot was successful, false otherwise
 */
bool rb_snapshot(RingBuffer *rb, double *buff_copy, int next_overlap_count);

/**
 * Resets the ring buffer
 *
 * @param cb Pointer to the RingBuffer to reset
 */
void rb_reset(RingBuffer *cb);

#endif // RING_BUFFER_H
