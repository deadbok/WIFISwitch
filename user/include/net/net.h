/** @file net.h
 *
 * @brief General network related interface.
 * 
 * Buffering for waiting send data. The SDK will fault, when trying to
 * send something before the last data has been send, and the sent
 * callback, has been invoked. This manages a ring buffer of
 * #NET_MAX_SEND_QUEUE items waiting to be send.
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
#ifndef NET_H
#define NET_H

#include <c_types.h>
#include <os_type.h>
//Should be included by espconn.h
#include <ip_addr.h>
#include <espconn.h>
#include "tools/dl_list.h"
#include "net/net-task.h"

/**
 * @brief Ring buffer for connections waiting to send data.
 */
#define NET_MAX_SEND_QUEUE 20

/**
 * @brief Connection types.
 */
enum 
{
	NET_CT_NONE,
	NET_CT_TCP,
	NET_CT_HTTP,
	NET_CT_WS,
	NET_CT_UDP,
	NET_CT_DNS,
	N_NET_CT
	
};

/**
 * @brief Array of string names for the connection types above.
 */
extern const char *net_ct_names[];
	

/**
 * @brief Array to look up a string from connection state.
 */
extern const char *state_names[];

/**
 * @brief Signal for disconnecting a network connection.
 * 
 * Use in network callback functions instead of #net_disconnect.
 */
extern os_signal_t signal_net_disconnect;

/**
 * @brief Signal for sending data over a connection.
 */
extern os_signal_t signal_net_send;

//Forward declaration.
struct net_connection;

/**
 * @brief Define a type for the network control functions.
 * 
 * This is a function pointer to a function, that controls some aspect
 * of the connection, like closing it.
 */
/**
 * @todo Maybe have a data parameter, like being able to send a reason
 * a websocket connection.
 */
typedef void (*net_ctrlfunc)(struct net_connection *);

/**
 * @brief Control functions for handling network connections.
 * 
 * Functions for controlling the network connection. A unimplemented 
 * function may be set to NULL.
 */
struct net_ctrlfuncs
{
    /**
     * @brief Callback when a connection is made.
     */
    net_ctrlfunc close;
};
/**
 * @brief Define a type for the callback functions.
 * 
 * This is a function pointer to a callback function, that receives a pointer to 
 * the network connection data of the connection, that initialised the callback. 
 */
typedef void (*net_callback)(struct net_connection *);

/**
 * @brief Callback functions for handling network events.
 * 
 * Callback functions for network event handling. All callbacks are passed a pointer
 * to the `struct tcp_connection` structure, associated with the event. Not all callbacks
 * are used by all connection types. A callback may be NULL, guess what happens then.
 */
struct net_callback_funcs
{
    /**
     * @brief Callback when a connection is made.
     */
    net_callback connect_callback;
    /**
     * @brief Callback when disconnected.
     */
    net_callback disconnect_callback;
    /**
     * @brief Callback when a write has been done.
     */
	net_callback write_finish_fn;
    /**
     * @brief Callback when something has been received.
     */
    net_callback recv_callback;
    /**
     * @brief Callback when something has been sent.
     */
    net_callback sent_callback;
};

/**
 * @brief Data used by the callback functions.
 * 
 * Data passed from espconn to the current callback. *Members that are 
 * unused should be zeroed.*
 */
struct net_callback_data
{
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
 * @brief Structure to keep track of network connections.
 *
 * This is used to create a list of all active connections, with the needed
 * information in each node.
 */ 
struct net_connection
{
    /**
     * @brief Pointer to the `struct espconn` structure associated with the connection.
     */
    struct espconn *conn;
    /**
     * @brief Connection type.
     */
    unsigned char type;
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
    struct net_callback_data callback_data;
    /**
     * @brief Pointers to callback functions for handling connection states.
     */
    struct net_callback_funcs *callbacks;
    /**
     * @brief Pointer to functions that control the connection.
     */
    struct net_ctrlfuncs *ctrlfuncs;
    /**
     * @brief Is the connection closing.
     */
    bool closing;
    /**
     * @brief Milliseconds connection is closed.
     */
    unsigned int timeout;
    /**
     * @brief Milliseconds of inactivity on the connections.
     * 
     * Reset whenever the connection has activity.
     */
    unsigned int inactivity;
    /**
     * @brief A pointer for the user, never touched.
     */
    void *user;
    /**
     * @brief Pointers for the prev and next entry in the list.
     */
    DL_LIST_CREATE(struct net_connection);
};

/**
 * @brief Initialise networking functions.
 */
extern void init_net(void);
/**
 * @brief Send or buffer some data.
 * 
 * @param data Pointer to data to send.
 * @param len Length of data to send (in bytes).
 * @param connection SDK connection to use.
 * @return Bytes send of buffered.
 */
size_t net_send(char *data, size_t len, struct espconn *connection);
/**
 * @brief Send an item in the send buffer.
 * 
 * Include this in any network sent callback to send the next item
 * waiting.
 */
void net_sent_callback(void);
/**
 * @brief Tell if there is stuff being send.
 * 
 * @return `True` if sending, `false` otherwise.
 */
bool net_is_sending(void);
/**
 * @brief Disconnect a network connection.
 * 
 * @param connection Connection to disconnect.
 * 
 * **Do not call this in a network callback. Use #signal_net_disconnect
 * to signal a disconnect instead.**
 */
void net_disconnect(struct net_connection *connection);
/**
 * @brief Find a connection.
 * 
 * Find a connection using its remote IP and port.
 * 
 * @param connections Pointer to a list of `struct net_connection`'s.
 * @param conn Pointer to a `struct espconn`.
 * @param listening Includes listening connections if `true` else skip them.
 * @return Pointer to a struct #tcp_connection for conn.
 */
extern struct net_connection *net_find_connection(struct net_connection *connections, struct espconn *conn);
/**
 * @brief Find a connection using its local port.
 *
 * @param connections Pointer to a list of `struct net_connection`'s.
 * @param conn Pointer to a `struct espconn`.
 * @return Pointer to a struct #tcp_connection for conn.
 */
extern struct net_connection *net_find_connection_by_port(struct net_connection *connections, unsigned int port);
/**
 * @brief Add a connection toa list of connections.
 *
 * @param connections Pointer to a list of `struct net_connection`'s.
 * @param connection Pointer to the connection to add.
 */
extern void net_add_connection(struct net_connection **connections, struct net_connection *connection);
//Save some space if we're not in debug mode.
#ifdef DEBUG
/**
 * @brief Print the status of all connections in the list.
 */
extern void net_print_connection_status(struct net_connection *connections);
#else
#define net_print_connection_status(...)
#endif

#endif
