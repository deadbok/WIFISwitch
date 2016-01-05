/**
 * @file udp.c
 *
 * @brief UDP routines for the ESP8266.
 * 
 * Abstract the espconn interface and manage multiply connection.
 * 
 * Notes:
 *  - I have no idea how many connections are possible.
 *  - It seems espconn works on top of lwip, and that it might be better to talk 
 *    to lwip itself.
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
#include "tools/missing_dec.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "ip_addr.h"
#include "mem.h"
#include "user_config.h"
#include "driver/uart.h"
#include "net/net.h"
#include "net/udp.h"

                  
/**
 * @brief Number of active connections.
 */
static unsigned char n_udp_connections;
/**
 * @brief Doubly-linked list of active connections.
 */
static struct net_connection *udp_connections = NULL;

//Forward declaration of callback functions.
static void udp_recv_cb(void *arg, char *data, unsigned short length);
static void udp_sent_cb(void *arg);

/**
 * @brief Internal callback, called when data is received.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void udp_recv_cb(void *arg, char *data, unsigned short length)
{
    struct espconn  *conn = arg;
    struct net_connection *connection;
    
    debug("UDP received (%p) %d bytes.\n", arg, length);
#ifdef DEBUG
    db_hexdump(data, length);
#endif
    
    connection = net_find_connection_by_port(udp_connections, conn->proto.udp->local_port);
    
	if (connection)
	{
		//Something has happened.
		connection->timeout = 0;
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct net_callback_funcs));
		//Set new data
		connection->callback_data.data = data;
		connection->callback_data.length = length;

		debug(" Listener found %p.\n", connection);
		//Call callback.
		if (connection->callbacks->recv_callback != NULL)
		{
			connection->callbacks->recv_callback(connection);
		}
		debug("Leaving UDP receive call back (%p).\n", conn);
		return;
	}
	warn("Could not find receiving connection.\n");
	debug("Leaving UDP receive call back (%p).\n", conn);
}

/**
 * @brief Internal callback, called when UDP data has been sent.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void udp_sent_cb(void *arg)
{
    struct espconn *conn = arg;
    struct net_connection *connection;
    
    debug("UDP sent (%p).\n", conn);
    
    net_find_connection_by_port(udp_connections, conn->proto.udp->local_port);

	if (connection)
	{
		//Something has happened.
		connection->timeout = 0;
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct net_callback_data));

		debug(" Listener found %p.\n", connection);
		//Call callback.
		if (connection->callbacks->sent_callback != NULL)
		{
			connection->callbacks->sent_callback(connection);
		}
		net_sent_callback();
		debug("Leaving UDP sent call back (%p).\n", conn);
		return;
	}
	warn("Could not find sending connection.\n");	
	debug("Leaving UDP sent call back (%p).\n", conn);
}

/**
 * @brief Create a listening connection.
 * 
 * This creates a listening UDP connection on a specific port.
 * 
 * @param port The port to listen to.
 * @param recv_cb Callback when something has been received.
 * @param sent_cb Callback when something has been sent.
 * @return `true` on success.
 */
bool udp_listen(unsigned int port,
								  net_callback recv_cb,
								  net_callback sent_cb)
{
    int ret;
    struct espconn *conn;
    struct net_callback_funcs *callbacks;
	struct net_connection *connections = udp_connections;
	struct net_connection *listening_connection = NULL;
    
    debug("Adding UDP listener on port %d.\n", port);
    udp_print_connection_status();
    //Check that nobody else is listening on the port.
    if (connections != NULL)
    {
		while (connections != NULL)
		{
			if ((connections->local_port == port) &&
				(connections->callbacks))
			{
				error("Port %d is in use.\n", port);
				return(false);
			}
			//Next listener.
			connections = connections->next;
		}
	}
	
	//Allocate memory for the new connection.
	listening_connection = (struct net_connection *)db_zalloc(sizeof(struct net_connection), "listening_connection udp_listen");
	if (!listening_connection)
	{
		error("Could not allocate memory for connection.\n");
		return(false);
	}
	listening_connection->conn = (struct espconn *)db_zalloc(sizeof(struct espconn), "espconn udp_listen");
	if (!listening_connection->conn)
	{
		error("Could not allocate memory for ESP connection.\n");
		return(false);
	}
	listening_connection->conn->proto.udp = (esp_udp *)db_zalloc(sizeof(esp_udp), "esp_udp udp_listen");
	if (!listening_connection->conn->proto.udp)
	{
		error("Could not allocate memory for UDP connection.\n");
		return(false);
	}
	listening_connection->callbacks = (struct net_callback_funcs *)db_zalloc(sizeof(struct net_callback_funcs), "callbacks udp_listen");
	if (!listening_connection->callbacks)
	{
		error("Could not allocate memory for UDP for call back pointers.\n");
		return(false);
	}
    
	conn = listening_connection->conn;
	callbacks = listening_connection->callbacks;
	
	//Setup UDP connection configuration.
	//Something has happened.
	listening_connection->timeout = 0;
	//UDP connection.
	conn->type = ESPCONN_UDP;
	listening_connection->type = NET_CT_UDP;
	
	//No state.
	conn->state = ESPCONN_NONE;
	//Set port.
	conn->proto.udp->local_port = port;
    listening_connection->local_port = port;

	//Setup user callbacks.
	callbacks->recv_callback = recv_cb;
	callbacks->sent_callback = sent_cb;
	
	//Register SDK callbacks.
	espconn_regist_recvcb(conn, udp_recv_cb);
	espconn_regist_sentcb(conn, udp_sent_cb);
		
	debug(" Accepting UDP connections on port %d...", port);
	ret = espconn_create(conn);
	if (ret != ESPCONN_OK)
	{
		debug("Error status %d.\n", ret);
		//Free unused memory.
		db_free(listening_connection);
		return(false);
	}
	debug("OK.\n");
	net_add_connection(&udp_connections, listening_connection);
    return(true);
}

/**
 * @brief Initialise UDP networking.
 * 
 * @return `true` on success.
 */
bool init_udp(void)
{
    debug("UDP init.\n");
    if (udp_connections != NULL)
    {
        error("UDP init already done.\n");
        return(false);
    }
        
    //No open connections.
    n_udp_connections = 0;
    udp_connections = NULL;
    
	return(true);
}

/**
 * @brief Close a listening UDP connection.
 * 
 * @param port Port to stop listening on.
 * @return True on success, false on failure.
 */
bool udp_stop(unsigned int port)
{
	int ret;
	struct net_connection *connection;
	
	debug("Stop listening for UDP on port %d.\n", port);
	
	connection = net_find_listening_connection_by_port(udp_connections, port);
	
	if (!connection)
	{
		warn("No listening connections.\n");
		return(false);
	}	
	
		debug("Closing UDP listening connection %p.\n", connection);
		ret = espconn_delete(connection->conn);
		debug(" Return status %d.\n", ret);
		db_free(connection);
		debug(" Connection data freed.\n");
		return(true);
}

/**
 * @brief Pointer to first connection in a list of all connections.
 * 
 * @return Pointer to a struct #net_connection.
 */
struct net_connection *udp_get_connections(void)
{
	return(udp_connections);
}

/** 
 * @brief Send UDP data.
 * 
 * *``udp_send`` clashes with lwip in SDK.*
 * 
 * @param connection Connection used for sending the data.
 * @param data Pointer to the data to send.
 * @param size Length of data in bytes.
 * 
 * @return true on success, false otherwise.
 */
bool db_udp_send(struct net_connection *connection, char *data, size_t size)
{
    debug("Sending %d bytes of UDP data (%p using %p),\n", size, data, connection);
	debug(" espconn pointer %p.\n", connection->conn);
	
	if (net_is_sending())
	{
		error(" Still sending something else.\n");
		return(false);
	}    
#ifdef DEBUG
	db_hexdump(data, size);
#endif //DEBUG
	if (connection->conn)
	{
		//net_sending = true;
		//connection->sending = true;
		return(net_send((unsigned char*)data, size, connection->conn));
	}
	warn(" Connection is empty.\n");
	return(false);
}

/**
 * @brief Disconnect the UDP connection.
 * 
 * *``udp_disconnect`` clashes with lwip in SDK.
 * 
 * @param connection Connection to disconnect.
 */
void db_udp_disconnect(struct net_connection *connection)
{
	int ret;
	
    debug("Disconnect (%p).\n", connection);
    debug(" espconn pointer %p.\n", connection->conn);
	connection->closing = true;
	ret = espconn_disconnect(connection->conn);
	debug(" Return status %d.\n", ret);
}

/** 
 * @brief Free up data structures used by a connection.
 * 
 * Free up the data pointed to by connection. espconn is expected to
 * clean up after itself.
 * @param connection Pointer to the data to free.
 */
 //TODO: Merge with TCP code into net_connection_free.
void udp_free(struct net_connection *connection)
{
	struct net_connection *connections = udp_connections;
	
    debug("Freeing up connection (%p).\n", connection);
    /* As far as I can tell the espconn library takes care of freeing up the
     * structures it has allocated. */

    if (connection != NULL)
    {
		//Remove connection from, the list of active connections.
		debug(" Unlinking.\n");
		DL_LIST_UNLINK(connection, connections);
		
		debug(" Deallocating connection data.\n");
        db_free(connection);
        
        //Free call backs if this is a listener.
        if (connection->callbacks)
        {
			debug( "Deallocating call back data.\n");
			db_free(connection->callbacks);
		}
		debug(" Connection deallocated.\n");
	    n_udp_connections--;
		debug(" Connections: %d.\n", n_udp_connections);
		return;
    }
    warn("Could not deallocate connection.\n");
}

#ifdef DEBUG
void udp_print_connection_status(void)
{
	debug("UDP connection(s):\n");
	net_print_connection_status(udp_connections);
}	
#endif
