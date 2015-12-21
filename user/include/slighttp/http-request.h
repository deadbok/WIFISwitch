/** 
 * @file http-request.h
 *
 * @brief Reqeust stuff for thw HTTP server.
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
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http.h"

extern bool http_parse_request(struct tcp_connection *connection, unsigned short length);
extern void http_free_request_headers(struct http_request *request):
extern void http_free_request(struct http_request *request);

#endif //HTTP_REQUEST_H
