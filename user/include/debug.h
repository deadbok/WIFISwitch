/** @file debug.h
 *
 * @brief Debug functions.
 * 
 * @copyright
 * Copyright 2016 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
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
#ifndef DEBUG_H
#define DEBUG_H

#include "fwconf.h"

/**
 * @brief Print an error message.
 */
#define error(...)     printf("ERROR(" __FILE__ "): " __VA_ARGS__ )

//Macro for debugging. Prints the message if warnings are enabled.
#ifdef WARNINGS
/**
 * @brief Print a warning message.
 */
#define warn(...)     printf("WARNING (" __FILE__ "): " __VA_ARGS__)
#else
#define warn(...)
#endif

//Macro for debugging. Prints the message if debugging is enabled.
#ifdef DEBUG
/**
 * @brief Print a debug message.
 */
#define debug(...)     printf(__VA_ARGS__)

//Hexdump some memory.
extern void db_hexdump(void *mem, unsigned int len);
#else
#define debug(...)
#define db_hexdump(mem, len)
#endif

//Debug memory de-/allocation if enabled.
#ifdef DEBUG_MEM
/**
 * @brief Maximum number of memory blocks to keep track of in debug mode.
 */
#define DBG_MEM_MAX_INFOS	200
#define db_malloc(ARG, INFO)  _db_alloc(ARG, false, INFO __FILE__)
#define db_zalloc(ARG, INFO)  _db_alloc(ARG, true, INFO __FILE__)
#define db_realloc(PTR, SIZE, INFO)  _db_realloc(PTR, SIZE, INFO __FILE__)
#define db_free(ARG)    _db_dealloc(ARG)             

extern void *_db_alloc(size_t size, bool zero, char *info);
extern void *_db_realloc(void *ptr, size_t size, char *info);
extern void _db_dealloc(void *ptr);
extern void db_mem_list(void);
                        
#else
#define db_malloc(ARG, INFO) malloc(ARG)
#define db_realloc(PTR, SIZE, INFO) realloc(PTR, SIZE)
#define db_free(ARG) free(ARG)
#define db_zalloc(ARG, INFO) zalloc(ARG)

//extern void *zalloc(size_t size);

#define db_mem_list(ARG)
#endif

#endif //DEBUG_H
