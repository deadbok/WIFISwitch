/** @file http-handler.h
 *
 * @brief Routines and structures for request handler registration.
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
#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "tools/dl_list.h"
#include "slighttp/http.h"

/**
 * @brief Return code when the handler is done.
 * 
 * This signals that the handler is done, and no one should take
 * over after it.
 */
#define RESPONSE_DONE_FINAL 0
/**
 * @brief Return code to move to next handler.
 * 
 * Signals that the handler is done, and another is allowed to
 * take over.
 * 
 * *To return this state, the handler **must not** have sent any
 * data*
 */
#define RESPONSE_DONE_CONTINUE -1
/**
 * @brief Return code when done.
 * 
 * Signals that the handler is done, and no one is allowed to
 * take over. The connection data must is kept, and some one else will
 * have to clean up.
 */
#define RESPONSE_DONE_NO_DEALLOC -2
/**
 * @brief Return code to when the handler has an error.
 */
#define RESPONSE_DONE_ERROR -3

extern bool http_add_handler(char *uri, http_handler_callback handler);
extern bool http_remove_handler(http_handler_callback handler);
extern http_handler_callback http_get_handler(
	struct http_request *request,
	http_handler_callback start_handler
);
extern signed int http_status_handler(struct http_request *request);
extern signed int ICACHE_FLASH_ATTR http_simple_GET_PUT_handler(
	struct http_request *request, 
	http_handler_callback get_cb,
	http_handler_callback put_cb,
	http_handler_callback free_cb
);

#endif
