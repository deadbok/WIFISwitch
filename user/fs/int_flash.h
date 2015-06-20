/**
 * @file int_flash.h
 *
 * @brief Routines for accessing the internal flash.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
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
 
#ifndef INT_FLASH_H
#define INT_FLASH_H

/**
 * @brief Address of the zip file.
 */
#define FS_ADDR 0x14000

//Interesting places.
extern void *_text_start;
extern void *_text_end;
extern void *_lit4_start;
extern void *_lit4_end;
extern void *_irom0_text_start;
extern void *_irom0_text_end;

union flash_data_u
{
	unsigned int u32;
	unsigned short u16[2];
	unsigned char u8[4];
};

//Flash access functions.
extern void flash_dump(unsigned int src_addr, size_t size);
extern void flash_dump_mem(unsigned int src_addr, size_t size);
extern bool aflash_read(const void *data, unsigned int read_addr, size_t size);


#endif
