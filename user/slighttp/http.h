/** @file http.h
 *
 * @brief Simple HTTP server for the ESP8266.
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
#ifndef HTTP_H
#define HTTP_H

#include "net/tcp.h"

/**
 * @brief Server name.
 */
#define HTTP_SERVER_NAME            "slighttpd"
/**
 * @brief Server version.
 */
#define HTTP_SERVER_VERSION         "0.0.4"
/**
 * @brief Version of HTTP that is supported.
 */
#define HTTP_SERVER_HTTP_VERSION    "1.1"
/**
 * @brief The largest block to read from a file, at a time.
 */
#define HTTP_FILE_CHUNK_SIZE  512
/**
 * @brief Size of send buffer.
 */
#define HTTP_SEND_BUFFER_SIZE 1024

/**
 * @brief HTTP request types.
 */
enum request_types
{
	/**
	 * @brief No type has been found yet.
	 */
	HTTP_NONE,
    HTTP_OPTIONS,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_TRACE,
    HTTP_CONNECT
};

/**
 * @brief HTTP response states.
 * 
 * Used to keep track of the progress when sending a response.
 */
enum response_states
{
	/**
	 * @brief Not doing anything.
	 */
	HTTP_STATE_NONE,
	/**
	 * @brief Looking for URI.
	 */
	HTTP_STATE_FIND,
	/**
	 * @brief Sending status line.
	 */
    HTTP_STATE_STATUS,
    /**
     * @brief Sending headers.
     */
    HTTP_STATE_HEADERS,
    /**
     * @brief Sending message.
     */
    HTTP_STATE_MESSAGE,
    /**
     * @brief The response has been assembled, and is being send.
     */
    HTTP_STATE_ASSEMBLED,
    /**
     * @brief All done,
     */
    HTTP_STATE_DONE,
    /**
     * @brief Error that selected handler couldn’t handle.
     */
    HTTP_STATE_ERROR 
};
/**
 * @brief A HTTP header entry.
 * 
 * To save space, the actual values are not copied here from the received data,
 * rather they are pointed to.
 */
struct http_header
{
    /**
     * @brief Pointer to the name.
     */
    char *name;
    /**
     * @brief Pointer to the value.
     */
    char *value;
};

/**
 * @brief Structure to keep the data of a HTTP response.
 */
struct http_response
{
    /**
     * @brief Numerical status code.
     */
    unsigned short status_code;
    /**
     * @brief How long we've gotten, in responding.
     */
     unsigned char state;
     /**
      * @brief Function pointers to handlers.
      */
     struct http_response_handler *handlers;
     /**
      * @brief Pointer to the context used by the sender.
      */
     void *context;
     /**
      * @brief Buffer with TCP data waiting to be send.
      */
     char *send_buffer;
     /**
      * @brief Pointer to the current position in the send buffer.
      */
     char *send_buffer_pos;
         /**
     * @brief Number of blocks is waiting to be send.
     */
    size_t send;
};

/**
 * @brief Structure to keep the data of a HTTP request.
 */
struct http_request
{
    /**
     * @brief Pointer to the connection data.
     */
    struct tcp_connection *connection;
    /**
     * @brief Type of HTTP request.
     */
    enum request_types type;
    /**
     * @brief The URI of the HTTP request.
     */
    char *uri;
    /**
     * @brief The version of the HTTP request.
     */
    char *version;
    /**
     * @brief Headers of the request.
     */
    struct http_header *headers;
    /**
     * @brief Number of headers
     */
     unsigned short n_headers;
    /**
     * @brief The message body of the HTTP request.
     */
    char *message;
    /**
     * @brief Response data for the request.
     */
    struct http_response response;
};

/**
 * @brief Callback function to test URIs.
 * 
 * This is called to test if the handler associated with it, will handle the URI.
 * 
 * @param uri The URI to generate the response message for.
 * @return True if we can handle the URI, false if not.
 */
typedef bool (*uri_comp_callback)(struct http_request *request);
/**
 * @brief Callback function for URIs.
 * 
 * This called to generate the response message. This functions is expected to
 * send anything needed for the response. *Care must be taken, not to overflow
 * the send buffer.* Therefore this function gets called until it sets 
 * `request->response.state`to #HTTP_STATE_DONE. `request->response.state`
 * is used to keep track of the progress, and must be updated.
 * If request->response.status_code is set to anything but 200, an error
 * response send instead.
 * 
 * @param request The request that led us here.
 * @return The size of the of what has been sent.
 */
typedef size_t (*uri_response_callback)(struct http_request *request);
/**
 * @brief Callback function to clean up after a response.
 */
typedef void (*uri_destroy_callback)(struct http_request *request);

/** 
 * @brief Structure to store response handlers.
 * 
 * Used to build an array of built in URI's that has generated responses. The
 * HTTP server will run through these URI's, when all other sources have failed.
 */
struct  http_response_handler
{
    /**
     * @brief A function to test if this handler will handle the URI.
     */
	uri_comp_callback will_handle;
	/**
     * @brief An array of function pointers, that sends a response.
     * 
     * One entry for each request method, in the same order as the
     * #request_type enum, except for the first (HTTP_NONE). The order is,
     * OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT. 
     */
    uri_response_callback handlers[8];
    /**
     * @brief A function pointer to a function, that does cleanup if needed.
     */
    uri_destroy_callback destroy;
};

extern char *http_fs_doc_root;

extern bool init_http(char *path, struct http_response_handler *handlers, unsigned short n_handlers);
extern struct http_response_handler *http_get_handlers(struct http_request *request);
extern unsigned char http_send(struct tcp_connection *connection, char *data, size_t size);

extern bool http_fs_test(struct http_request *request);
extern size_t http_fs_head_handler(struct http_request *request);
extern size_t http_fs_get_handler(struct http_request *request);
extern void http_fs_destroy(struct http_request *request);

#endif
