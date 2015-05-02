/* http.h
 *
 * Simple HTTP server for the ESP8266.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
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
#ifndef HTTP_H
#define HTTP_H

//Server name and version.
#define HTTP_SERVER_NAME            "slimhttp"
#define HTTP_SERVER_VERSION         "0.0.1"
//Version of HTTP that is supported.
#define HTTP_SERVER_HTTP_VERSION    "1.1"

//Responses
#define HTTP_200_OK                 "HTTP/" HTTP_SERVER_HTTP_VERSION \
                                    " 200 OK\r\nContent-Type: text/html\
                                    \r\nConnection: close\r\nContent-Length: "
#define HTTP_404_NOT_FOUND          "HTTP/" HTTP_SERVER_HTTP_VERSION \
                                    " 404 Not Found\r\nContent-Type: text/html\
                                    \r\nConnection: close\r\nContent-Length: "
#define HTTP_404_NOT_FOUND_HTML     "<!DOCTYPE html><head><title>Resource not found.\
                                    </title></head><body><h1>404 Not Found</h1>\
                                    <br /><br />Resource not found.</body></html>"                                    

/* Callback function for static URIs, that are served if no other way is
 * found of generating the response.
 */
typedef char *(*uri_callback)(char *uri);

struct  http_builtin_uri
{
	const char      *uri;
	uri_callback    callback;
};

void init_http(struct http_builtin_uri *builtin_uris, unsigned short n_builtin_uris);

#endif
