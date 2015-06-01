/** @file http-response.h
 *
 * @brief Response stuff for thw HTTP server.
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
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http.h"

/**
 * @brief Turn a preprocessor variable into a string.
 */
#define TOSTR(x)  #x
#define STRINGIFY(x)  TOSTR(x)

/**
 * @brief HTTP version part of the status-line response.
 */
#define HTTP_STATUS_HTTP_VERSION "HTTP/" HTTP_SERVER_HTTP_VERSION
/**
 * @brief Generate a status-line.
 */
#define HTTP_STATUS_LINE(CODE, MSG) HTTP_STATUS_HTTP_VERSION " " CODE " " MSG
/**
 * @brief Create a header.
 */
#define HTTP_HDR_CREATE(NAME, VALUE) {\
									  	NAME,\
										VALUE\
									 }
//Predefined response status-lines.
/**
 * @brief HTTP 200 OK response.
 */
#define HTTP_STATUS_200 HTTP_STATUS_LINE("200", "OK")
/**
 * @brief HTTP 400 bad request.
 */
#define HTTP_STATUS_400 HTTP_STATUS_LINE("400", "Bad Request")
/**
 * @brief HTTP 404 not found response.
 */
#define HTTP_STATUS_404 HTTP_STATUS_LINE("404", "Not Found")
/**
 * @brief HTTP 501 Not implemented response.
 */
#define HTTP_STATUS_501 HTTP_STATUS_LINE("501", "Not Implemented")

//Predefined header values
/**
 * @brief HTML content.
 */
#define HTTP_HDR_CONTENT_HTML HTTP_HDR_CREATE("Content-Type", "text/html")
/**
 * @brief Server name.
 */
#define HTTP_HDR_SERVER HTTP_HDR_CREATE("Server", HTTP_SERVER_NAME)
/**
 * @brief Close connection when done.
 */
#define HTTP_HDR_CONNECTION_CLOSE HTTP_HDR_CREATE("Connection", "close")

//Predefined HTML for responses
/**
* @brief HTTP 400 response HTML.
 */
#define HTTP_400_HTML               "<!DOCTYPE html><head><title>Bad Request.\
                                    </title></head><body><h1>400 Bad Request</h1>\
                                    <br />Sorry I didn't quite get that.</body></html>"
#define HTTP_400_HTML_LENGTH		135
/**
 * @brief HTTP 404 response HTML.
 */
#define HTTP_404_HTML               "<!DOCTYPE html><head><title>Resource not found.</title></head><body><h1>404 Not Found</h1><br />Resource not found.</body></html>"                                    
#define HTTP_404_HTML_LENGTH		129
/**
 * @brief HTTP 501 response HTML.
 */
#define HTTP_501_HTML               "<!DOCTYPE html><head><title>Resource not found.\
                                    </title></head><body><h1>501 Not Implemented</h1>\
                                    <br />Don't know what to say.</body></html>"
#define HTTP_501_HTML_LENGTH		139

extern void http_send_response(struct http_response *response, bool send_content);
extern struct http_response *http_generate_response(
									struct tcp_connection *connection,
									unsigned short status_code);
extern void http_free_response(struct http_response *response);

#endif //HTTP_RESPONSE_H
