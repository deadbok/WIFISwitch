/* int_flash.h
 *
 * Routines for accessing the internal flash.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
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

#include "memzip.h"
 
extern void *_text_start;
extern void *_text_end;
extern void *_lit4_start;
extern void *_lit4_end;
extern void *_irom0_text_start;
extern void *_irom0_text_end;
 
extern void flash_dump(unsigned int src_addr, unsigned int size);
extern void *read_flash(size_t size);
extern MEMZIP_FILE_HDR *load_header(unsigned int address);
 
#endif
