/** 
 * @file sha1.h
 *
 * @brief SHA1 hash function (RFC3174).
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
#ifndef SHA1_H
#define SHA1_H

#include <stdbool.h>

/**
 * @brief Use this to create a multi-type array.
 * 
 * Creates an array where elements are accessible as both 8-,
 * 32-, and 64-bit entries.
 */
#define sha1_type_u(words) union                                       \
{                                                                      \
	uint8_t b[words << 2];                                             \
	uint32_t dw[words];                                                \
	uint64_t qw[words >> 1];                                           \
}

/**
 * @brief Context used by the SHA1 routines.
 */	
struct sha1_context
{
	/**
	 * @brief Hash values.
	 */
	sha1_type_u(5) h;
	/**
	 * @brief Length of the message.
	 */
	sha1_type_u(2) length;
	/**
	 * @brief Final digest.
	 */
	sha1_type_u(5) digest;
	/**
	 * @brief Buffer used during computation.
	 */
	sha1_type_u(80) buffer;
	/**
	 * @brief True if the end bit needed.
	 */
	bool end_bit;
	/**
	 * @brief True if a final padding chunk is needed.
	 */
	bool pad;
};

/**
 * @brief Initialise the SHA1 context.
 * 
 * @param context Pointer to a sha1_context.
 */
extern void sha1_init(struct sha1_context *context);
/**
 * @brief Process a chunk of the message.
 * 
 * @param w Maximum of 64 bytes of the message.
 * @param size Bits in the message.
 * @param context Pointer to the corresponding context.
 */
extern void sha1_process(uint32_t w[16], uint64_t size, struct sha1_context *context);
/**
 * @brief Finalise hashing, and save result in context.
 * 
 * @param context Pointer to the context.
 */
extern void sha1_final(struct sha1_context *context);

#endif
