/** @file http-common.h
 *
 * @brief Common stuff for the HTTP server.
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
#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H
#include "slighttp/http-request.h"

/**
 * @brief Eat initial spaces.
 * 
 * Eat initial spaces in a string replacing then with '\0', and advance the pointer.
 */
#define HTTP_EAT_SPACES(string) while (*string == ' ')\
                                    *string++ = '\0'
/**
 * @brief Skip initial spaces.
 * 
 * Skip initial spaces in a string.
 */
#define HTTP_SKIP_SPACES(string) while (*string == ' ')                \
									*string++

/**
 * @brief Print a Common Log Format message to the console.
 * 
 * Log access to the web server to console in a standardized text file format:
 * 
 *  host ident authuser date request status bytes
 * 
 * or:
 * 
 *  127.0.0.1 user-identifier frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326
 * 
 * See [CLF](https://en.wikipedia.org/wiki/Common_Log_Format).
 */
extern void http_print_clf_status(struct http_request *request);
/**
 * @brief Eat line ends.
 * 
 * Replace occurrences of both CRLF and LF by ``\0``.
 * 
 * @param ptr The string to work on.
 * @param number Number of CRLF's to eat.
 */
extern char *http_eat_crlf(char *ptr, size_t number);
/**
 * @brief Skip line ends.
 * 
 * Return a pointer to just after the lene end.
 * 
 * @param ptr The string to work on.
 * @param number Number of CRLF's to eat.
 * @return Pointer after the line end.
 */
extern char *http_skip_crlf(char *ptr, size_t number);

#endif //HTTP_COMMON_H
