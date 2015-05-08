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

/**
 * @brief Server name.
 */
#define HTTP_SERVER_NAME            "slimhttp"
/**
 * @brief Server version.
 */
#define HTTP_SERVER_VERSION         "0.0.1"
/**
 * @brief Version of HTTP that is supported.
 */
#define HTTP_SERVER_HTTP_VERSION    "1.1"

/**
 * @brief HTTP 200 OK response.
 */
#define HTTP_200_OK                 "HTTP/" HTTP_SERVER_HTTP_VERSION \
                                    " 200 OK\r\nContent-Type: text/html\
                                    \r\nConnection: close\r\nContent-Length: "
/**
 * @brief HTTP 404 not found response.
 */
#define HTTP_404_NOT_FOUND          "HTTP/" HTTP_SERVER_HTTP_VERSION \
                                    " 404 Not Found\r\nContent-Type: text/html\
                                    \r\nConnection: close\r\nContent-Length: "
/**
 * @brief HTTP 404 response HTML.
 */
#define HTTP_404_NOT_FOUND_HTML     "<!DOCTYPE html><head><title>Resource not found.\
                                    </title></head><body><h1>404 Not Found</h1>\
                                    <br /><br />Resource not found.</body></html>"                                    

/**
 * @brief Callback function for static URIs.
 * 
 * These are called if no other way is found of generating the response.
 */
typedef char *(*uri_callback)(char *uri);

/** 
 * @brief Structure to store information for a built in URI.
 * 
 * Used to build an array of built in URI's that has generated responses. The
 * HTTP server will run through these URI's, when all other sources have failed.
 */
struct  http_builtin_uri
{
    /**
     * @brief The URI that the callback can answer.
     */
	const char      *uri;
	/**
     * @brief A function pointer to a function, that renders the answer.
     */
    uri_callback    callback;
};

void init_http(struct http_builtin_uri *builtin_uris, unsigned short n_builtin_uris);

#endif
