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
 * @brief Eat line ends from HTTP headers.
 * 
 * This macro is used to be tolerant, accepting both CRLF and LF endings.
 */
#define HTTP_EAT_CRLF(ptr, number)  do\
                                    {\
                                        if (*ptr == '\r')\
                                        {\
                                            os_bzero(ptr, number * 2);\
                                            ptr += (number * 2);\
                                        }\
                                        else\
                                        {\
                                            os_bzero(ptr, number);\
                                            ptr += number;\
                                        }\
                                    } while(0)
/* Wrapping stuff in do...while(0), makes it possible to invoke the macro like a
 * function call ending with semicolon (;).
 * Idea from here: [https://gcc.gnu.org/onlinedocs/cpp/Swallowing-the-Semicolon.html
 */
/**
 * @brief Skip initial spaces.
 * 
 * Skip initial spaces in a string.
 */
#define HTTP_SKIP_SPACES(string) while (*string == ' ')\
                                    string++
/**
 * @brief Skip line ends from HTTP headers.
 * 
 * This macro is used to be tolerant, accepting both CRLF and LF endings.
 */
#define HTTP_SKIP_CRLF(ptr, number)  do\
                                    {\
                                        if (*ptr == '\r')\
                                        {\
                                            ptr += (number * 2);\
                                        }\
                                        else\
                                        {\
                                            ptr += number;\
                                        }\
                                    } while(0)

extern void http_print_clf_status(struct http_request *request);

#endif //HTTP_COMMON_H
