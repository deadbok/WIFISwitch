/** @file tcp.h
 *
 * @brief TCP routines for the ESP8266.
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
#ifndef TCP_H
#define TCP_H

//Should be included by espconn.h
#include <ip_addr.h>
#include <espconn.h>
#include "net/net.h"

/**
 * @brief Default time out in milliseconds of all TCP connections.
 */
#define TCP_TIMEOUT 60000

extern void tcp_free(struct net_connection *connection);
extern struct net_connection *tcp_listen(unsigned int port, net_callback connect_cb,  
                                net_callback disconnect_cb, 
                                net_callback write_finish_cb, 
                                net_callback recv_cb, 
                                net_callback sent_cb);
extern bool tcp_send(struct net_connection *connection, char *data, size_t size);
extern bool init_tcp(void);
extern bool tcp_stop(unsigned int port);
extern struct net_connection *tcp_get_connections(void);
//Save some space if we're not in debug mode.
#ifdef DEBUG
/**
 * @brief Print the status of all connections in the list.
 */
extern void tcp_print_connection_status(void);
#else
#define tcp_print_connection_status(...)
#endif

#endif
