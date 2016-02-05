/** @file ring.c
 *
 * @brief Ring buffer helpers.
 * 
 * @copyright
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
 * @license
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */ 
#include "user_interface.h"
#include "user_config.h"
#include "tools/ring.h"

void init_ring(struct ring_buffer *rb, size_t size, size_t capacity)
{
	debug("Creating ring buffer at %p.\n", rb);
	rb->data = db_malloc(size * capacity, "rb->data init_ring");
	rb->capacity = capacity;
	rb->count = 0;
	rb->head = rb->data;
	rb->tail = rb->data;
	rb->item_size = size;
	debug(" Item size: %d.\n", size);
	debug(" Capacity: %d items.\n", capacity);
	debug(" Buffer size: %d bytes.\n", size*capacity); 
}

void *ring_push_back(struct ring_buffer *rb)
{
	void *ret;
	
	debug("Getting empty item in ring buffer %p.\n", rb);
	rb->count++;
	
	//Check for overflow.
	if (rb->count == rb->capacity)
	{
		rb->count--;
		debug(" Buffer is full.\n");
		return(NULL);
	}

	ret = rb->tail;
	rb->tail = rb->tail + rb->item_size;
	//Handle wrap around.
	if (rb->tail > (rb->data + ((rb->capacity - 1) * rb->item_size)))
	{
		debug(" Reached the end of the buffer array.\n");
		//Go back to the start.
		rb->tail = rb->data;
	}
	debug(" Got %d of %d items: %p.\n", rb->count, rb->capacity, ret);

	return(ret);
}

void *ring_pop_front(struct ring_buffer *rb)
{
	void *new_head;
	void *ret;
	
	debug("Getting next item in ring buffer %p.\n", rb);

	//Check for underflow.
	if (!rb->count)
	{
		debug(" Buffer is empty.\n");
		return(NULL);
	}

	//Handle wrap around.
	new_head = rb->head + rb->item_size;
	if (new_head > (rb->data + ((rb->capacity - 1) * rb->item_size)))
	{
		debug(" Reached the end of the buffer array.\n");
		//Go back to the start.
		new_head = rb->data;
	}

	debug(" Got %d of %d items: %p.\n", rb->count, rb->capacity, rb->head);
	rb->count--;
	
	ret = db_malloc(rb->item_size, "ret ring_pop_front");
	os_memcpy(ret, rb->head, rb->item_size);
	debug(" Returning copy at %p.\n", ret);
	//Point head at next item.
	rb->head = new_head;
	
	return(ret);
}
