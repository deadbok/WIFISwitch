/** @file http-tcp.h
 *
 * @brief TCP stuff for the HTTP server.
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
#ifndef HTTP_TCP_H
#define HTTP_TCP_H
#include "tools/ring.h"

extern struct ring_buffer request_buffer;
extern int http_response_mutex;

extern void tcp_connect_cb(struct tcp_connection *connection);
extern void tcp_reconnect_cb(struct tcp_connection *connection);
extern void tcp_disconnect_cb(struct tcp_connection *connection);
extern void tcp_write_finish_cb(struct tcp_connection *connection);
extern void tcp_recv_cb(struct tcp_connection *connection);
extern void tcp_sent_cb(struct tcp_connection *connection);

#endif //HTTP_TCP_H
