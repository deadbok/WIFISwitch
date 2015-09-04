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
#include "missing_dec.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "ip_addr.h"
#include "mem.h"
#include "user_config.h"
#include "driver/uart.h"
#include "udp.h"

                  
/**
 * @brief Number of active connections.
 */
static unsigned char n_udp_connections;
/**
 * @brief Doubly-linked list of active connections.
 */
static struct udp_connection *udp_connections = NULL;

//Forward declaration of callback functions.
static void ICACHE_FLASH_ATTR udp_recv_cb(void *arg, char *data, unsigned short length);
static void ICACHE_FLASH_ATTR udp_sent_cb(void *arg);

/**
 * @brief Find a connection using its local port.
 * 
 * @param conn Pointer to a `struct espconn`.
 * @return Pointer to a struct #udp_connection for conn.
 */
static struct udp_connection ICACHE_FLASH_ATTR *find_listening_connection(unsigned int port)
{
	struct udp_connection *connection = udp_connections;

	if (connection == NULL)
	{
		warn("No connections.\n");
		return(NULL);
	}

	debug(" Looking for listening connection on port %d.\n", port);

    while (connection != NULL)
    {
		debug("Connection %p (%p).\n", connection, connection->conn);
		if ((connection->local_port == port) &&
			(connection->callbacks))
		{
			debug(" Connection found %p.\n", connection);
			return(connection);
		}
		connection = connection->next;
	}
	warn("Connection not found.\n");
	return(NULL);
}

/**
 * @brief Add a connection to the list of active connections.
 * 
 * @param connection Pointer to the connection to add.
 */
static void ICACHE_FLASH_ATTR add_active_connection(struct udp_connection *connection)
{
	struct udp_connection *connections = udp_connections;
	
	debug("Adding %p to active connections.\n", connection);
	//Add the connection to the list of active connections.
	if (connections == NULL)
	{
		udp_connections = connection;
		connection->prev = NULL;
		connection->next = NULL;
	}
	else
	{
		DL_LIST_ADD_END(connection, connections);
	}
	n_udp_connections++;
	debug(" Connections: %d.\n", n_udp_connections);
}

/**
 * @brief Print the status of all connections in the list.
 */
void ICACHE_FLASH_ATTR udp_print_connection_status(void)
{
	struct udp_connection *connection = udp_connections;
	unsigned int connections = 0;
	
	if (!connection)
	{
		debug("No UDP connections.\n");
	}
	else
	{
		while (connection)
		{
			connections++;
			if (connection->conn->state == ESPCONN_NONE)
			{
				debug("UDP onnection %p (%p).\n", connection, connection->conn);
				debug(" Remote address " IPSTR ":%d.\n", 
					  IP2STR(connection->remote_ip),
					  connection->remote_port);
				debug(" SDK remote address " IPSTR ":%d.\n", 
					  IP2STR(connection->conn->proto.udp->remote_ip),
					  connection->conn->proto.udp->remote_port);
			}
			connection = connection->next;
		}
		if (connections == 1)
		{
			debug("1 UDP connection.\n");
		}
		else
		{
			debug("%d UDP connections.\n", connections);
		}
	}
}

/**
 * @brief Internal callback, called when data is received.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void ICACHE_FLASH_ATTR udp_recv_cb(void *arg, char *data, unsigned short length)
{
    struct espconn  *conn = arg;
    struct udp_connection *connection;
    
    debug("UDP received (%p) %d bytes.\n", arg, length);
    db_hexdump(data, length);
    udp_print_connection_status();
    
    connection = find_listening_connection(conn->proto.udp->local_port);
    
	if (connection)
	{
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct udp_callback_data));
		//Set new data
		connection->callback_data.arg = arg;
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
static void ICACHE_FLASH_ATTR udp_sent_cb(void *arg)
{
    struct espconn *conn = arg;
    struct udp_connection *connection;
    
    debug("UDP sent (%p).\n", conn);
    udp_print_connection_status();
    
    connection = find_listening_connection(conn->proto.udp->local_port);

	if (connection)
	{
		connection->sending = false;
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct udp_callback_data));
		//Set new data
		connection->callback_data.arg = arg;

		debug(" Listener found %p.\n", connection);
		//Call callback.
		if (connection->callbacks->sent_callback != NULL)
		{
			connection->callbacks->sent_callback(connection);
		}
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
bool ICACHE_FLASH_ATTR udp_listen(unsigned int port,
								  udp_callback recv_cb,
								  udp_callback sent_cb)
{
    int ret;
    struct espconn *conn;
    struct udp_callback_funcs *callbacks;
	struct udp_connection *connections = udp_connections;
	struct udp_connection *listening_connection = NULL;
    
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
	listening_connection = (struct udp_connection *)db_zalloc(sizeof(struct udp_connection), "listening_connection udp_listen");
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
	listening_connection->callbacks = (struct udp_callback_funcs *)db_zalloc(sizeof(struct udp_callback_funcs), "callbacks udp_listen");
	if (!listening_connection->callbacks)
	{
		error("Could not allocate memory for UDP for call back pointers.\n");
		return(false);
	}
    
	conn = listening_connection->conn;
	callbacks = listening_connection->callbacks;
	
	//Setup UDP connection configuration.
	//UDP connection.
	conn->type = ESPCONN_UDP;
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
	add_active_connection(listening_connection);
    return(true);
}

/**
 * @brief Initialise UDP networking.
 * 
 * @return `true` on success.
 */
bool ICACHE_FLASH_ATTR init_udp(void)
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
bool ICACHE_FLASH_ATTR udp_stop(unsigned int port)
{
	int ret;
	struct udp_connection *listening_connection;
	
	debug("Stop listening for UDP on port %d.\n", port);
	
	listening_connection = find_listening_connection(80);
	
	if (!listening_connection)
	{
		warn("No listening connections.\n");
		return(false);
	}	
	
		debug("Closing UDP listening connection %p.\n", listening_connection);
		ret = espconn_delete(listening_connection->conn);
		debug(" Return status %d.\n", ret);
		db_free(listening_connection);
		debug(" Connection data freed.\n");
		return(true);
}

/**
 * @brief Pointer to first connection in a list of all connections.
 * 
 * @return Pointer to a struct #udp_connection.
 */
struct udp_connection ICACHE_FLASH_ATTR *udp_get_connections(void)
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
bool ICACHE_FLASH_ATTR db_udp_send(struct udp_connection *connection, char *data, size_t size)
{
    debug("Sending %d bytes of UDP data (%p using %p),\n", size, data, connection);
	debug(" espconn pointer %p.\n", connection->conn);
	
	if (connection->sending)
	{
		error(" Still sending something else.\n");
		return(false);
	}    
#ifdef DEBUG
	db_hexdump(data, size);
#endif //DEBUG
	if (connection->conn)
	{
		connection->sending = true;
		return(espconn_send(connection->conn, (unsigned char*)data, size));
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
void ICACHE_FLASH_ATTR db_udp_disconnect(struct udp_connection *connection)
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
void ICACHE_FLASH_ATTR udp_free(struct udp_connection *connection)
{
	struct udp_connection *connections = udp_connections;
	
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
