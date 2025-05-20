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
