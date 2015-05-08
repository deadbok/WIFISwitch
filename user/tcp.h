/** @file tcp.h
 *
 * @brief TCP server routines for the ESP8266.
 *
 * @copyright
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
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

#include "espconn.h"
#include "dl_list.h"

/**
 * Maximum number of TCP connections supported.
 */
#define TCP_MAX_CONNECTIONS 5

/**
 * @brief Data used by the callback functions.
 * 
 * Data passed from espconn to the current callback. *Members that are unused 
 * are zeroed.*
 */
struct tcp_callback_data
{
    /**
     * @brief espconn callback `arg` parameter.
     * 
     * Usually a pointer to a `struct espconn`.
     */ 
    void            *arg;
    /**
     * @brief espconn callback `data` parameter.
     * 
     * Used by the receive callback for passing received data.
     */ 
    char            *data;
    /**
     * @brief Length of the data pointed to by meḿber `data`.
     */
    unsigned short  length;
    /**
     * @brief espconn callback `err` parameter.
     * 
     * Used by the reconnect callback to tell what's up. Equals an `ESPCONN_*`
     * status.
     */
    err_t            err;
};

struct tcp_connection;

typedef void (*tcp_callback)(struct tcp_connection *);


/**
 * @brief Structure to keep track of TCP connections.
 *
 * This is used to create a list of all active connections, with the needed
 * information in each node.
 */ 
struct tcp_connection
{
    /**
     * @brief Pointer to the `struct espconn` structure associated with the connection.
     */
    struct espconn              *conn;
    /**
     * @brief Pointer to the data meant for the current callback.
     */
    struct tcp_callback_data    callback_data;
    DL_LIST_CREATE(struct tcp_connection);
};

void tcp_listen(int port, tcp_callback connect_cb, 
                                tcp_callback reconnect_cb, 
                                tcp_callback disconnect_cb, 
                                tcp_callback write_finish_cb, 
                                tcp_callback recv_cb, 
                                tcp_callback sent_cb);
char tcp_send(struct tcp_connection *connection, char * const data);
char tcp_disconnect(struct tcp_connection *connection);
void init_tcp(void);

#endif
