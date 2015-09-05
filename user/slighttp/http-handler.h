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
#include "slighttp/http-request.h"

/**
 * @brief Callback issued when the request line is parsed.
 * 
 * @param request Request data.
 * @return True if we can handle the URI, false if not.
 */
typedef bool (*http_request_callback)(struct http_request *request);
/**
 * @brief Callback used when ever a header is found in the request.
 * 
 * @param request Request data parsed so far.
 * @param header_line The header line that caused the callback.
 */
typedef void (*http_request_hdr_callback)(struct http_request *request, char *header_line);
/**
 * @brief Callback function for URIs.
 * 
 * This called to generate the response message. This functions is expected to
 * send anything needed for the response. *Care must be taken, not to overflow
 * the send buffer.* Therefore this function gets called until it sets 
 * `request->response.state`to #HTTP_STATE_DONE. `request->response.state`
 * is used to keep track of the progress, and must be updated.
 * If request->response.status_code is set to anything but 200, an error
 * response is send instead.
 * 
 * @param request The request that led us here.
 * @return The size of the of what has been sent.
 */
typedef signed int (*http_response_callback)(struct http_request *request);
/**
 * @brief Callback function to clean up after a response.
 */
typedef void (*http_destroy_callback)(struct http_request *request);

/** 
 * @brief Structure to store response handlers.
 * 
 * Used to build an array of built in URI's that has generated responses. The
 * HTTP server will run through these URI's, when all other sources have failed.
 */
struct http_response_handler
{
    /**
     * @brief A function to test if this handler will handle the URI.
     * 
     * This is called as soon as the request line has been parsed.
     */
	http_request_callback request_cb;
    /**
     * @brief A function to handle the headers in a request.
     * 
     * This is called as soon as the request headers are available.
     * *Any header data you wish to keep, should be copied, as the
     * request parser discard these data, before calling the method
     * callbacks.* 
     */
	http_request_hdr_callback header_cb;
	/**
     * @brief An array of function pointers, that sends a response.
     * 
     * One entry for each request method, in the same order as the
     * #request_type enum, except for the first (HTTP_NONE). The order is,
     * OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT. 
     * 
     * One of these is called when the request has been parsed
     * completely.
     * 
     * To not overflow the buffer of the ESP8266 the handlers are
     * expected to not send large chunks of data in one go. The
     * callback is invoked multiply times, while the value of
     * request->response.state changes. The callback should return
     * the size of data send (max. 1440 bytes I think), and 0 when
     * done. The callback is expected (increase) to adjust the response
     * state, informing the response logic, of the current state.
     * 
     * If 0 is returned the state is expected to be done, and slighttp
     * will move on to the handler set by request->reponse.state. If
     * less than zero is returned slighttp will stop calling the
     * response handler.
     */
    http_response_callback method_cb[8];
    /**
     * @brief A function pointer to a function, that does cleanup if needed.
     */
    http_destroy_callback destroy_cb;
};

extern bool http_add_handler(char *uri, struct http_response_handler *handler);
extern bool http_remove_handler(struct http_response_handler *handler);
extern struct http_response_handler *http_get_handler(struct http_request *request);
extern void http_default_header_handler(struct http_request *request, char *header_line);

#endif
