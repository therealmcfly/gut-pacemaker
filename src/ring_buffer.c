#include "ring_buffer.h"

#define WRITE_COUNT_INIT_VAL -1 // initially set to -1 because first time it is full, it should not be considered a write count
#define READY_TO_READ_INIT_VAL 0
#define IS_FULL_INIT_VAL 0

// Initialize buffer
void rb_init(RingBuffer *rb)
{
	rb->size = BUFFER_SIZE;

	rb->head = rb->buffer;
	rb->tail = rb->buffer;
	rb->end = rb->buffer + rb->size;
	rb->is_full = READY_TO_READ_INIT_VAL;
	rb->ready_to_read = READY_TO_READ_INIT_VAL;
	rb->write_count = WRITE_COUNT_INIT_VAL;
	printf("Circular buffer initialized.\n");
}

// Check if full
bool rb_is_full(RingBuffer *rb)
{
	double *next = rb->head + 1;
	if (next == rb->end)
		next = rb->buffer;
	return next == rb->tail;
}

bool full_check(RingBuffer *rb)
{
	return rb->head == rb->tail;
}

// Check if empty
bool rb_is_empty(RingBuffer *rb)
{
	return rb->head == rb->tail;
}

// Push data
bool rb_push(RingBuffer *rb, double data)
{
	// *(cb->head) = data;
	// // if head is at the end, wrap around
	// if (cb->head == cb->end)
	// 	cb->head = cb->buffer;
	// else
	// 	// Otherwise, just advance the head
	// 	cb->head++;

	// If full, advance tail to discard the oldest value
	if (rb_is_full(rb))
	{
		rb->tail++;
		if (rb->tail == rb->end)
			rb->tail = rb->buffer;
	}

	*(rb->head) = data;

	rb->head++;
	if (rb->head == rb->end)
		rb->head = rb->buffer;

	return false;
}

// Pop data
bool rb_pop(RingBuffer *rb, double *data)
{
	if (rb_is_empty(rb))
		return false;

	*data = *(rb->tail);
	rb->tail++;
	if (rb->tail == rb->end)
		rb->tail = rb->buffer;
	return true;
}

// Peek data
bool rb_peek(RingBuffer *rb, double *data)
{
	if (rb_is_empty(rb))
		return false;

	*data = *(rb->tail);
	return true;
}

void rb_push_sample(RingBuffer *rb, double data)
{

	*(rb->head) = data;

	// printf("\nStored %f at %p\n", *(cb->head), cb->head);
	// int stored_index = cb->head - cb->buffer + 1;
	// printf("\nStored %f at buf_num [%d]\n", *(cb->head), stored_index);

	rb->head++;
	rb->write_count++;

	if (rb->head == rb->end)
	{
		rb->head = rb->buffer;
		rb->is_full = true;
	}

	// printf("Next head: %p\n", cb->head);
	// printf("Next head value: %f at %p\n", *(cb->head), cb->head);
	// printf("Tail value: %f at %p\n", *(cb->tail), cb->tail);

	// int next_index = cb->head - cb->buffer + 1;
	// int tail_index = cb->tail - cb->buffer + 1;

	// printf("Next head value: %f at buf_num [%d]\n", *(cb->head), next_index);
	// printf("Tail value: %f at buf_num [%d]\n", *(cb->tail), tail_index);
}

void rb_reset(RingBuffer *rb)
{
	rb->head = rb->buffer;
	rb->tail = rb->buffer;
	rb->end = rb->buffer + BUFFER_SIZE;
	rb->is_full = IS_FULL_INIT_VAL;
	rb->ready_to_read = READY_TO_READ_INIT_VAL;
	rb->write_count = WRITE_COUNT_INIT_VAL;
}

void rb_snapshot(RingBuffer *rb, double *buff_copy, int next_overlap_count)
{
	printf("Taking snapshot of ring buffer!\n");

	double *curr = rb->tail;
	for (int i = 0; i < rb->size; i++)
	{
		buff_copy[i] = *curr;
		curr++;
		if (curr == rb->end)
			curr = rb->buffer;
	}

	rb->write_count = 0;
	rb->ready_to_read = false;

	// tail is set to the start index of the next buffer
	rb->tail = rb->tail + next_overlap_count;
	if (rb->tail >= rb->end)
	{
		rb->tail = rb->buffer + ((rb->tail - rb->end) % rb->size);
	}
}
