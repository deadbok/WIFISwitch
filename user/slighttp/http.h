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
#define HTTP_SERVER_VERSION         "0.0.7"
/**
 * @brief Version of HTTP that is supported.
 */
#define HTTP_SERVER_HTTP_VERSION    "1.1"
/**
 * @brief The largest block to read from a file, at a time.
 */
#define HTTP_FILE_CHUNK_SIZE  1440
/**
 * @brief Size of send buffer.
 */
#define HTTP_SEND_BUFFER_SIZE 1440
/**
 * @brief Number of request that can be buffered.
 */
#define HTTP_REQUEST_BUFFER_SIZE 50

//Forward declaration.
struct http_request;

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
 * @brief Callback function for HTTP handlers.
 * 
 * This called to generate the response message or modify the 
 * request/response in some way, before calling another handler. If the
 * handler send a response, *care must be taken, not to overflow
 * the send buffer*.
 * The handler is called repeatedly until the return value is less than
 * 1. This makes it possible to send the data in chunks.
 * 
 * @param request The request that led us here.
 * @return The size of the of what has been sent, or one of the 
 *         RESPONSE_DONE_* values.
 */
typedef signed int (*http_handler_callback)(struct http_request *request);

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
     http_handler_callback handler;
     /**
      * @brief Pointer to the context used by the sender.
      */
     void *context;
     /**
      * @brief Buffer with TCP data waiting to be send.
      */
     char send_buffer[HTTP_SEND_BUFFER_SIZE];
     /**
      * @brief Pointer to the current position in the send buffer.
      */
     char *send_buffer_pos;
     /**
      * @brief Number of recursion levels in response handler.
      */
     unsigned char level;
     /**
      * @brief Size of the message.
      */
     signed long message_size;
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
     * @brief Copy of the header data received.
     */
    char *headers;
    /**
     * @brief The message body of the HTTP request.
     */
    char *message;
    /**
     * @brief Response data for the request.
     */
    struct http_response response;
};

extern char *http_fs_doc_root;

extern bool init_http(unsigned int port);
extern bool http_get_status(void);
extern size_t http_send(struct tcp_connection *connection, char *data, size_t size);

#endif
