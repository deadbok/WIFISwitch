/** 
 * @file sha1.c
 *
 * @brief SHA1 hash function (RFC3174).
 *
 * Coded looking at the pseudo code on the wikipedia.
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
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "user_config.h"
#include "sha1.h"

/**
 * @brief SHA1 circular left shift macro.
 */
#define SHA1_S(b, w) (((w) << (b)) | ((w) >> (32 - (b))))

/**
 * @brief Endian swap an unsigned 32 bit int.
 */
#define SHA1_SWAP_32(v) ((v) << 24) |                                  \
                        (((v) & 0x0000ff00) << 8) |                    \
                        (((v) & 0x00ff0000) >> 8) |                    \
                        ((v) >> 24)
/**
 * @brief Endian swap an unsigned 64 bit int.
 */   
#define SHA1_SWAP_64(v) ((v) << 56) |                                  \
                        (((v) & 0x000000000000ff00) << 40) |           \
                        (((v) & 0x0000000000ff0000) << 24) |           \
                        (((v) & 0x00000000ff000000) <<  8) |           \
                        (((v) & 0x000000ff00000000) >>  8) |           \
                        (((v) & 0x0000ff0000000000) >> 24) |           \
                        (((v) & 0x00ff000000000000) >> 40) |           \
                        ((v) >> 56)


/**
 * @brief Pad to 512 bits according to specs.
 * 
 * @param context Pointer to the SHA1 context.
 * @param size Size of message in bits.
 * @return True if done, False if a final padding block is needed.
 */
static bool sha1_pad(struct sha1_context *context, uint64_t size)
{
	size_t offset = size >> 3;
	
	debug("Padding input to 512 bits at 0x%04x.\n", (unsigned int)offset);

	if (offset < 56)
	{
		//1-Bit;
		context->buffer.b[offset++] = 0x80;
	}
	else
	{
		debug(" Need to add another chunk.\n");
		//Need another chunk to finish this.
		os_bzero(context->buffer.b + offset, 64 - offset);
		return(false);
	}

	//Zero padding, just do the rest of the block.
	os_bzero(context->buffer.b  + offset, 64 - offset);
	//Lengths.
	context->buffer.qw[7] = SHA1_SWAP_64(context->length.qw[0]);
	
	return(true);
}

/**
 * @brief Do the actual processing of a message.
 * 
 * Do the SHA1 computation of a msg that is padded correctly.
 * 
 * @param context Pointer to SHA1 context.
 */
static void sha1_process_chunk(struct sha1_context *context)
{
	uint32_t a, b, c, d, e, f, k, temp;
	unsigned char i;
	
	debug(" Processing chunk from %p.\n", context);
	for (i = 0; i < 16; i++)
	{
		context->buffer.dw[i] = SHA1_SWAP_32(context->buffer.dw[i]);
	}
	
	for (i = 16; i < 80; i++)
	{
        context->buffer.dw[i] = SHA1_S(1, (context->buffer.dw[i-3] ^ context->buffer.dw[i-8] ^ context->buffer.dw[i-14] ^ context->buffer.dw[i-16]));
	}

	a = context->h.dw[0];
	b = context->h.dw[1];
	c = context->h.dw[2];
	d = context->h.dw[3];
	e = context->h.dw[4];
	for (i = 0; i < 80; i++)
	{
        if (i <= 19)
        {
			f = (b & c) | ((~b) & d);
            k = 0x5A827999;
		}
        else if ((20 <= i) && (i <= 39))
        {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
		}
        else if ((40 <= i) && (i <= 59))
        {
            f = (b & c) | (b & d) | (c & d) ;
            k = 0x8F1BBCDC;
		}
        else if ((60 <= i) && (i <= 79))
        {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
		}
		temp = SHA1_S(5, a) + f + e + k + context->buffer.dw[i];
        e = d;
        d = c;
        c = SHA1_S(30, b);
        b = a;
        a = temp;
	}
	context->h.dw[0] += a;
	context->h.dw[1] += b;
	context->h.dw[2] += c;
	context->h.dw[3] += d;
	context->h.dw[4] += e;
	
	os_bzero(context->buffer.b, 64);
	debug("Done.\n");
}

void sha1_init(struct sha1_context *context)
{
	debug("Initialising SHA1 hashing context %p.\n", context);
	
	//Set initial hash values.
	context->h.dw[0] = 0x67452301;
	context->h.dw[1] = 0xEFCDAB89;
	context->h.dw[2] = 0x98BADCFE;
	context->h.dw[3] = 0x10325476;
	context->h.dw[4] = 0xC3D2E1F0; 	
	
	context->length.qw[0] = 0;
	os_bzero(context->buffer.b, 80);
	os_bzero(context->digest.b, 20);
	
	//Makes padding a zero byte message work, padding by default.
	context->pad = true;
}

void sha1_process(uint32_t w[16], uint64_t size, struct sha1_context *context)
{
	debug("Processing chunk, " PRIu64 " bits.\n", size);
	debug(" Current length " PRIu64 ".\n", context->length.qw[0]);
	
	//Copy the message into the buffer.
	os_memcpy(context->buffer.b, w, size >> 3);
	context->length.qw[0] += size;
	if (size == 512)
	{
		debug(" Chunk %p.\n", w);
		db_hexdump(context->buffer.b, 64);
		sha1_process_chunk(context);
		context->pad = true;
	}
	else
	{	
		context->pad = !sha1_pad(context, size);
		if (context->pad)
		{
			debug(" Overflow to new chunk.\n");
		}
		debug(" Chunk %p.\n", w);
		db_hexdump(context->buffer.b, 64);
		sha1_process_chunk(context);
	}
	
	debug(" Current length " PRIu64 ".\n", context->length.qw[0]);
	return;
}

void sha1_final(struct sha1_context *context)
{
	unsigned char i;
	
	debug("Finalising digest %p.\n", context);
	debug(" Final length " PRIu64 ".\n", context->length.qw[0]);
	
	if (context->pad)
	{
		debug(" Padding chunk.\n");
		sha1_pad(context, 0);
		db_hexdump(context->buffer.b, 64);
		sha1_process_chunk(context);		
	}

	for (i = 0; i < 5; i++)
	{
		//Swap the hash values into the final digest.
		context->digest.dw[i] = SHA1_SWAP_32(context->h.dw[i]);
	}
}
	
