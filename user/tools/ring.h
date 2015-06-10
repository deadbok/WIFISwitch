/** @file ring.h
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
#ifndef RING_H
#define RING_H

/**
 * @brief Ring buffer structure.
 */
struct ring_buffer
{
	/**
	 * @brief Pointer to the data in the buffer.
	 */
	void *data;
	/**
	 * @brief Pointer to the first item of the buffer.
	 */
	 void *head;
	 /**
	  * @brief Pointer to the last used item in buffer.
	  */
	 void *tail;
	 /**
	  * @brief Capacity of the buffer.
	  */
	 size_t capacity;
	 /**
	  * @brief Items in the buffer.
	  */
	 size_t count;
	 /**
	  * @brief Size of an item.
	  */
	 size_t item_size;
};

extern void init_ring(struct ring_buffer *rb, size_t size, size_t capacity);
extern void *ring_push_back(struct ring_buffer *rb);
extern void *ring_pop_front(struct ring_buffer *rb);

#endif
