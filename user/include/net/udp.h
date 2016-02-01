/** @file udp.h
 *
 * @brief UDP routines for the ESP8266.
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
#ifndef UDP_H
#define UDP_H

//Should be included by espconn.h
#include "ip_addr.h"
#include "espconn.h"
#include "tools/dl_list.h"
#include "net/net.h"

extern const char *state_names[];

extern void udp_print_connection_status(void);
extern void udp_free(struct net_connection *connection);
extern bool udp_listen(unsigned int port,  
                                net_callback recv_cb, 
                                net_callback sent_cb);
extern bool db_udp_send(struct net_connection *connection, char *data, size_t size);
extern bool init_udp(void);
extern bool udp_stop(unsigned int port);
extern struct net_connection *udp_get_connections(void);

//Save some space if we're not in debug mode.
#ifdef DEBUG
/**
 * @brief Print the status of all connections in the list.
 */
extern void udp_print_connection_status(void);
#else
#define udp_print_connection_status(...)
#endif

#endif
