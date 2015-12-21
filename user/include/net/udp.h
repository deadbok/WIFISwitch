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

//Forward declarations.
struct udp_connection;

/**
 * @brief Data used by the callback functions.
 * 
 * Data passed from espconn to the current callback. *Members that are 
 * unused should be zeroed.*
 */
struct udp_callback_data
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
};

/**
 * @brief Define a type for the callback functions.
 * 
 * This is a function pointer to a callback function, that receives a pointer to 
 * the UDP connection data of the connection, that initialised the callback. 
 */
typedef void (*udp_callback)(struct udp_connection *);

/**
 * @brief Callback functions for handling UDP events.
 * 
 * Callback functions for UDP event handling. All callbacks are passed a pointer
 * to the `struct udp_connection` structure, associated with the event.
 */
struct udp_callback_funcs
{
    /**
     * @brief Callback when something has been received.
     */
    udp_callback recv_callback;
    /**
     * @brief Callback when something has been sent.
     */
    udp_callback sent_callback;
};

/**
 * @brief Structure to keep track of UDP connections.
 *
 * This is used to create a list of all active connections, with the needed
 * information in each node.
 */ 
struct udp_connection
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
    struct udp_callback_data callback_data;
    /**
     * @brief Pointers to callback functions for handling connection states.
     * 
     * This is NULL on all but the listening connections.
     */
    struct udp_callback_funcs *callbacks;
    /**
     * @brief Is the connection sending data.
     */
    //bool sending;
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
    DL_LIST_CREATE(struct udp_connection);
};

extern const char *state_names[];

extern void udp_print_connection_status(void);
extern void udp_free(struct udp_connection *connection);
extern bool udp_listen(unsigned int port,  
                                udp_callback recv_cb, 
                                udp_callback sent_cb);
extern bool db_udp_send(struct udp_connection *connection, char *data, size_t size);
extern void db_udp_disconnect(struct udp_connection *connection);
extern bool init_udp(void);
extern bool udp_stop(unsigned int port);
extern struct udp_connection *udp_get_connections(void);

#endif
