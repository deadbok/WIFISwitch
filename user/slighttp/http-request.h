/** @file http-response.h
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

extern void http_handle_request(struct tcp_connection *connection,
										   unsigned short error_code, 
										   bool headers_only);
extern char *http_parse_headers(struct tcp_connection *connection,
								char* raw_headers);
extern char *http_parse_request(struct tcp_connection *connection, 
								unsigned char start_offset);
extern void http_process_request(struct tcp_connection *connection);
#endif //HTTP_REQUEST_H
