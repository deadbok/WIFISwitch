/** 
 * @file common.h
 *
 * @brief Common routines.
 * 
 * *This tool is meant for small file systems used on embedded
 * systems, and keeps everything in memory while building the
 * image.*
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
#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

/**
 * @brief Be verbose if set.
 */
extern bool verbose;

/* Wrapping stuff in do...while(0), makes it possible to invoke the macro like a
 * function call ending with semicolon (;).
 * Idea from here: [https://gcc.gnu.org/onlinedocs/cpp/Swallowing-the-Semicolon.html
 */
#define info(...) do\
				  {\
					  if (verbose) \
					  {\
						  printf(__VA_ARGS__);\
					  }\
				   }\
				   while (0)\

/**
 * @brief Exit with failure status.
 * 
 * @todo: Deallocate memory gracefully.
 *
 * @param message Error message.
 */
extern void die(const char* message);

#endif //COMMON_H
