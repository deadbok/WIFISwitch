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
#include "ip_addr.h"
#include "espconn.h"
#include "tools/dl_list.h"

//Forward declarations.
struct tcp_connection;

/**
 * @brief Data used by the callback functions.
 * 
 * Data passed from espconn to the current callback. *Members that are 
 * unused should be zeroed.*
 */
struct tcp_callback_data
{
    /**
     * @brief espconn callback `arg` parameter.
     * 
     * Usually a pointer to a `struct espconn`.
     */ 
    //void            *arg;
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

/**
 * @brief Define a type for the callback functions.
 * 
 * This is a function pointer to a callback function, that receives a pointer to 
 * the TCP connection data of the connection, that initialised the callback. 
 */
typedef void (*tcp_callback)(struct tcp_connection *);

/**
 * @brief Callback functions for handling TCP events.
 * 
 * Callback functions for TCP event handling. All callbacks are passed a pointer
 * to the `struct tcp_connection` structure, associated with the event.
 */
struct tcp_callback_funcs
{
    /**
     * @brief Callback when a connection is made.
     */
    tcp_callback connect_callback;
    /**
     * @brief Callback when disconnected.
     */
    tcp_callback disconnect_callback;
    /**
     * @brief Callback when a write has been done.
     */
	tcp_callback write_finish_fn;
    /**
     * @brief Callback when something has been received.
     */
    tcp_callback recv_callback;
    /**
     * @brief Callback when something has been sent.
     */
    tcp_callback sent_callback;
};

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
    struct espconn *conn;
    /**
     * @brief Remote IP.
     */
    uint8 remote_ip[4];
    /**
     * @brief Remote port.
     */
	unsigned int remote_port;
    /**
     * @brief Local IP.
     */
	uint8 local_ip[4];
    /**
     * @brief Local port.
     */
    unsigned int local_port;
    /**
     * @brief Pointer to the data meant for the current callback.
     */
    struct tcp_callback_data callback_data;
    /**
     * @brief Pointers to callback functions for handling connection states.
     */
    struct tcp_callback_funcs *callbacks;
    /**
     * @brief Is the connection sending data.
     */
    bool sending;
    /**
     * @brief Is the connection closing.
     */
    bool closing;
    /**
     * @brief A pointer for the user, never touched.
     */
    void *user;
    /**
     * @brief Pointers for the prev and next entry in the list.
     */
    DL_LIST_CREATE(struct tcp_connection);
};

extern const char *state_names[];

extern void tcp_free(struct tcp_connection *connection);
extern bool tcp_listen(unsigned int port, tcp_callback connect_cb,  
                                tcp_callback disconnect_cb, 
                                tcp_callback write_finish_cb, 
                                tcp_callback recv_cb, 
                                tcp_callback sent_cb);
extern bool tcp_send(struct tcp_connection *connection, char *data, size_t size);
extern void tcp_disconnect(struct tcp_connection *connection);
extern bool init_tcp(void);
extern bool tcp_stop(unsigned int port);
extern struct tcp_connection *tcp_get_connections(void);

//Save some space if we're not in debug mode.
#ifdef DEBUG
extern void tcp_print_connection_status(void);
#else
#define tcp_print_connection_status(...)
#endif

#endif
