#ifndef RING_BUFFER_H
#define RING_BUFFER_H

// Circular buffer size

typedef struct
{
	// double buffer[SIGNAL_PROCESSING_BUFFER_SIZE]; // Fixed-size buffer
	double *buffer;				// Pointer to the buffer array
	int size;							// Size of the buffer array
	double *head;					// Write pointer
	double *tail;					// Read pointer
	double *end;					// Pointer to end of the buffer(next address after the last element))
	int is_full;					// Flag to indicate if the buffer is full
	int rtr_flag;					// Flag to indicate if the buffer is ready
	int new_signal_count; // count of write after last read
} RingBuffer;

/**
 * Initializes the ring buffer
 *
 * @param rb Pointer to the RingBuffer to initialize
 * @param buffer Pointer to the buffer array
 * @param buffer_size Size of the buffer array
 */
void rb_init(RingBuffer *rb, double *buffer, int buffer_size);
/**
 * Pushes a sample into the ring buffer
 *
 * @param rb Pointer to the RingBuffer to push data into
 * @param data The sample data to push
 * @return true if the sample was successfully pushed, false otherwise
 */
int rb_push_sample(RingBuffer *rb, double data);

/**
 * Takes a snapshot of the ring buffer
 *
 * @param rb Pointer to the RingBuffer to snapshot
 * @param buff_copy Pointer to the buffer to copy data into
 * @param next_overlap_count Number of samples to overlap in the next buffer
 * @param reset_flag Function pointer to reset the flag
 * @return true if the snapshot was successful, false otherwise
 */
int rb_snapshot(RingBuffer *rb, double *buff_copy, int next_overlap_count);

/**
 * Resets the ring buffer
 *
 * @param rb Pointer to the RingBuffer to reset
 */
void rb_reset(RingBuffer *rb);

#endif // RING_BUFFER_H
