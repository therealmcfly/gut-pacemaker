#include "ring_buffer.h"

#include <stdio.h>
#include <stddef.h> // for NULL

#define WRITE_COUNT_INIT_VAL 0
#define READY_TO_READ_INIT_VAL 0
#define IS_FULL_INIT_VAL 0

// Initialize buffer
void rb_init(RingBuffer *rb)
{
	rb->size = BUFFER_SIZE;

	rb->head = rb->buffer;
	rb->tail = rb->buffer;
	rb->end = rb->buffer + rb->size;
	rb->is_full = IS_FULL_INIT_VAL;
	rb->rtr_flag = READY_TO_READ_INIT_VAL;
	rb->new_signal_count = WRITE_COUNT_INIT_VAL;
	printf("Ring buffer initialized.\n");
}

bool rb_push_sample(RingBuffer *rb, double data)
{
	if (rb == NULL)
	{
		fprintf(stderr, "Error: NULL pointer passed to rb_push_sample\n");
		return false;
	}

	// add data to head
	*(rb->head) = data;

	// shift head to next index
	rb->head++;
	if (rb->head == rb->end) // if head points over the last index
	{
		rb->head = rb->buffer; // set head to 0th index
		if (rb->is_full == false)
			rb->is_full = true;
	}

	rb->new_signal_count++;
	return true;
}

void rb_reset(RingBuffer *rb)
{
	rb->head = rb->buffer;
	rb->tail = rb->buffer;
	rb->end = rb->buffer + BUFFER_SIZE;
	rb->is_full = IS_FULL_INIT_VAL;
	rb->rtr_flag = READY_TO_READ_INIT_VAL;
	rb->new_signal_count = WRITE_COUNT_INIT_VAL;
}

bool rb_snapshot(RingBuffer *rb, double *buff_copy, int next_overlap_count)
{
	if (rb == NULL || buff_copy == NULL)
	{
		fprintf(stderr, "Error: NULL pointer passed to rb_snapshot\n");
		return false;
	}

	if (next_overlap_count < 0 || next_overlap_count > rb->size)
	{
		fprintf(stderr, "Error: Invalid overlap count in rb_snapshot\n");
		return false;
	}
	rb->rtr_flag = false; // reset rtr_flag flag

	printf("Taking snapshot of ring buffer!\n");
	// rtr_flag is reset outside this function after
	double *curr = rb->tail;
	for (int i = 0; i < rb->size; i++)
	{
		buff_copy[i] = *curr;
		curr++;
		if (curr == rb->end)
			curr = rb->buffer;
	}

	// tail is set to the start index of the next buffer
	rb->tail = rb->tail + next_overlap_count;
	if (rb->tail >= rb->end)
	{
		rb->tail = rb->buffer + ((rb->tail - rb->end) % rb->size);
	}
	return true;
}
