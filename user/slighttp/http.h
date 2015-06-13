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
 * @brief The largest block to read from a file, at a time.
 */
#define HTTP_FILE_CHUNK_SIZE  256

/**
 * @brief Server name.
 */
#define HTTP_SERVER_NAME            "slighttpd"
/**
 * @brief Server version.
 */
#define HTTP_SERVER_VERSION         "0.0.3"
/**
 * @brief Version of HTTP that is supported.
 */
#define HTTP_SERVER_HTTP_VERSION    "1.1"
/**
 * @brief HTTP request types.
 */
enum request_type
{
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
    enum request_type   type;
    /**
     * @brief The URI of the HTTP request.
     */
    char                *uri;
    /**
     * @brief The version of the HTTP request.
     */
    char                *version;
    /**
     * @brief Headers of the request.
     */
    struct http_header  *headers;
    /**
     * @brief Number of headers
     */
     unsigned short     n_headers;
    /**
     * @brief The message body of the HTTP request.
     */
    char                *message;
};
/**
 * @brief Structure to keep the data of a HTTP response.
 */
#if 0
struct http_response
{
    /**
     * @brief Pointer to the connection data.
     */
    struct tcp_connection *connection;
    /**
     * @brief Numerical status code.
     */
     unsigned short status_code;
    /**
     * @brief Status-line.
     */
     char *status_line;
    /**
     * @brief The start line of the response.
     */
    struct http_header *headers;
    /**
     * @brief Number of headers
     */
     unsigned short     n_headers;
    /**
     * @brief The message body of the HTTP response.
     */
    char                *message;
};
#endif

/**
 * @brief Callback function for static URIs.
 * 
 * This called to generate the response message. This functions is expected to
 * send any headers and the message for the response. The function should 
 * return the size of the response message in bytes.
 * 
 * @param uri The uri to generate the response message for.
 * @param request The request that led us here.
 * @return The size of the response message send.
 */
typedef size_t (*uri_response_callback)(char *uri, struct http_request *request);
/**
 * @brief Callback function to test static URIs.
 * 
 * This is called to test if the handler associated with it, will handle the URI.
 * 
 * @param uri The URI to generate the response message for.
 * @return True if we can handle the URI, false if not.
 */
typedef bool (*uri_comp_callback)(char *uri);
/**
 * @brief Callback function to clean up after a response.
 */
typedef void (*uri_destroy_callback)(void);

/** 
 * @brief Structure to store information for a built in URI.
 * 
 * Used to build an array of built in URI's that has generated responses. The
 * HTTP server will run through these URI's, when all other sources have failed.
 */
struct  http_builtin_uri
{
    /**
     * @brief A function to test if this handler will handle the URI.
     */
	uri_comp_callback test_uri;
	/**
     * @brief A function pointer to a function, that renders the answer.
     */
    uri_response_callback handler;
    /**
     * @brief A function pointer to a function, that cdoes cleanup if needed.
     */
    uri_destroy_callback destroy;
};

extern char *http_fs_doc_root;

extern void init_http(char *path, struct http_builtin_uri *builtin_uris, unsigned short n_builtin_uris);

#endif
