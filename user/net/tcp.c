/**
 * @file tcp.c
 *
 * @brief TCP routines for the ESP8266.
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
#include "net/tcp.h"
               
/**
 * @brief Number of active connections.
 */
static unsigned char n_tcp_connections = 0;
/**
 * @brief Doubly-linked list of active connections.
 */
static struct net_connection *tcp_connections = NULL;
/**
 * @brief Number of listening connections.
 */
static unsigned char n_tcp_listening_connections = 0;
/**
 * @brief Doubly-linked list of listening connections.
 */
static struct net_connection *tcp_listening_connections = NULL;
/**
 * @brief Structure with TCP connection control functions.
 */
static struct net_ctrlfuncs tcp_ctrlfuncs = { tcp_disconnect };

//Forward declaration of callback functions.
static void tcp_connect_cb(void *arg);
static void tcp_reconnect_cb(void *arg, sint8 err);
static void tcp_disconnect_cb(void *arg);
static void tcp_write_finish_cb(void *arg);
static void tcp_recv_cb(void *arg, char *data, unsigned short length);
static void tcp_sent_cb(void *arg);

/**
 * @brief Internal callback for when a new connection has been made.
 * 
 * @param arg Pointer to an espconn connection structure.
 * 
 * Uses the callbacks from the listening connection by default.
 */
static void tcp_connect_cb(void *arg)
{
	struct espconn *conn = arg;
    struct net_connection *connection;
    struct net_connection *listening_connection;
    
    debug("TCP connected (%p).\n", conn);
    tcp_print_connection_status();
    
    listening_connection = net_find_connection_by_port(tcp_listening_connections, conn->proto.tcp->local_port);
    
	connection = (struct net_connection *)db_zalloc(sizeof(struct net_connection), "connection tcp_connect_cb");
	connection->conn = arg;
	
	connection->callbacks = (struct net_callback_funcs *)db_zalloc(sizeof(struct net_callback_funcs), "connection->callbacks tcp_connect_cb");
	if (!connection->callbacks)
	{
		error("Could not allocate memory for TCP for call back pointers.\n");
		return;
	}
	os_memcpy(connection->callbacks, listening_connection->callbacks, sizeof(struct net_callback_funcs));
	
	//Register SDK callbacks.
	espconn_regist_connectcb(connection->conn, tcp_connect_cb);
	espconn_regist_reconcb(connection->conn, tcp_reconnect_cb);
	espconn_regist_disconcb(connection->conn, tcp_disconnect_cb);
	espconn_regist_write_finish(connection->conn, tcp_write_finish_cb);
	espconn_regist_recvcb(connection->conn, tcp_recv_cb);
	espconn_regist_sentcb(connection->conn, tcp_sent_cb);

	connection->ctrlfuncs = &tcp_ctrlfuncs;
	
	//TCP connection.
	connection->type = NET_CT_TCP;
	//Nothing's happening.
	connection->closing = false;
	//Something has happened.
	connection->timeout = TCP_TIMEOUT;
	connection->inactivity = 0;
	
	//Save connection addresses.
	os_memcpy(connection->local_ip, conn->proto.tcp->local_ip, 4);
	connection->local_port = conn->proto.tcp->local_port;
	os_memcpy(connection->remote_ip, conn->proto.tcp->remote_ip, 4);
	connection->remote_port = conn->proto.tcp->remote_port;
	
	debug(" Remote address " IPSTR ":%d.\n", 
		  IP2STR(connection->remote_ip),
		  connection->remote_port);

	net_add_connection(&tcp_connections, connection);
	n_tcp_connections++;
	
	//Call callback.
	connection->callbacks->connect_callback(connection);

	debug("Leaving TCP connect call back (%p).\n", conn);
}

/**
 * @brief Generic code for reconnect and disconnect callback.
 * 
 * Since I handle both events the same way, the code is in this
 * function to avoid duplication.
 * 
 * @return True on success false otherwise.
 */
static bool tcp_handle_disconnect(struct net_connection *connection)
{
	debug("Handling TCP re/dis-connect.\n");
	if (connection)
	{
		//Something has happened.
		connection->inactivity = 0;
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct net_callback_data));

		//Call callback.
		if (connection->callbacks->disconnect_callback != NULL)
		{
			connection->callbacks->disconnect_callback(connection);
		}
		tcp_free(connection);
		return(true);
	}
	return(false);
}
	
/**
 * @brief Internal error handler.
 * 
 * Called whenever an error has occurred. This is like the disconnect
 * call in that all sorts of stuff, could have happened to the espconn
 * struct, so this handled as a disconnect.
 * 
 * @param arg Pointer to an espconn connection structure.
 * @param err Error code.
 */
static void tcp_reconnect_cb(void *arg, sint8 err)
{
    struct espconn *conn = arg;
    struct net_connection *connection;
            
    debug("TCP reconnected (%p) status %d.\n", conn, err);
    tcp_print_connection_status();
    
    connection = net_find_connection(tcp_connections, conn);
    
    debug("Handling as disconnect.\n");
    
    if (tcp_handle_disconnect(connection))
    {
 		debug("Leaving TCP reconnect call back (%p).\n", conn);
		return;
	}
	warn("Could not find reconnected connection.\n");
	debug("Leaving TCP reconnect call back (%p).\n", conn);
}

/**
 * @brief Internal callback for when someone disconnects.
 *
 * This call seems to be more like: "Hey! I have disconnected
 * but since I have already freed all data, I'm returning the
 * parameters in the listening connection ."
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void tcp_disconnect_cb(void *arg)
{
    struct espconn *conn = arg;
    struct net_connection *connection;
            
    debug("TCP disconnected (%p).\n", arg);
    tcp_print_connection_status();
    
    connection = net_find_connection(tcp_connections, conn);
    
    if (tcp_handle_disconnect(connection))
    {
		debug("Leaving TCP disconnect call back (%p).\n", conn);
		return;
	}
	debug("Could not find disconnected connection.\n");
	debug("Leaving TCP disconnect call back (%p).\n", conn);
}

/**
 * @brief Internal callback, called when a TCP write is done.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void tcp_write_finish_cb(void *arg)
{
    struct espconn *conn = arg;
    struct net_connection *connection;
    
    debug("TCP write done (%p).\n", arg);
    tcp_print_connection_status();
    
    connection = net_find_connection(tcp_connections, conn);
    
	if (connection)
	{
		//Something has happened.
		connection->inactivity = 0;
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct net_callback_data));

		//Call callback.
		if (connection->callbacks->write_finish_fn != NULL)
		{
			connection->callbacks->write_finish_fn(connection);
		}
		debug("Leaving TCP write finish call back (%p).\n", conn);
		return;
	}
	warn("Could not find connection.\n");
	debug("Leaving TCP write finish call back (%p).\n", conn);
}

/**
 * @brief Internal callback, called when data is received.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void tcp_recv_cb(void *arg, char *data, unsigned short length)
{
    struct espconn  *conn = arg;
    struct net_connection *connection;
    
    debug("TCP received (%p).\n", arg);
    debug("%s\n", data);
    tcp_print_connection_status();
    
    connection = net_find_connection(tcp_connections, conn);
	if (connection)
	{
		//Something has happened.
		connection->inactivity = 0;  

		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct net_callback_data));
		//Set new data
		connection->callback_data.data = data;
		connection->callback_data.length = length;

		//Call callback.
		if (connection->callbacks->recv_callback != NULL)
		{
			debug(" Entering callback %p.\n", connection->callbacks->recv_callback);
			connection->callbacks->recv_callback(connection);
		}
		debug("Leaving TCP receive call back (%p).\n", conn);
		return;
	}
	warn("Could not find receiving connection.\n");
}

/**
 * @brief Internal callback, called when TCP data has been sent.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void tcp_sent_cb(void *arg)
{
    struct espconn *conn = arg;
    struct net_connection *connection;
	    
    debug("TCP sent (%p).\n", conn);
    tcp_print_connection_status();
    
    connection = net_find_connection(tcp_connections, conn);
    net_sent_callback();

	if (connection)
	{
		//Something has happened.
		connection->inactivity = 0;
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct net_callback_data));

		//Call callback.
		if (connection->callbacks->sent_callback != NULL)
		{
			connection->callbacks->sent_callback(connection);
		}
		//Call send buffer handler.
		debug("Leaving TCP sent call back (%p).\n", conn);
		return;
	}
	warn("Could not find sending connection.\n");	
	debug("Leaving TCP sent call back (%p).\n", conn);
}

/**
 * @brief Create a listening connection.
 * 
 * This creates a listening TCP connection on a specific port.
 * 
 * @param port The port to listen to.
 * @param connect_cb Callback when a connection is made.
 * @param reconnect_cb Callback on a reconnect. According to the SDK 
 *                     documentation, this should be considered an error
 *                     callback. If an error happens somewhere in the espconn 
 *                     code, this is called.
 * @param disconnect_cb Callback when disconnected.
 * @param write_finish_cb Callback when a write has been done.
 * @param recv_cb Callback when something has been received.
 * @param sent_cb Callback when something has been sent.
 * @return Pointer to struct net_connection, or NULL on failure.
 */
struct net_connection *tcp_listen(unsigned int port,
				net_callback connect_cb, 
				net_callback disconnect_cb,
				net_callback write_finish_cb,
				net_callback recv_cb,
				net_callback sent_cb)
{
    int ret;
    struct espconn *conn;
    struct net_callback_funcs *callbacks;
	struct net_connection *connections = tcp_connections;
	struct net_connection *listening_connection = NULL;
    
    debug("Adding TCP listener on port %d.\n", port);
    tcp_print_connection_status();
    //Check that nobody else is listening on the port.
    debug(" Checking if port is in use.\n");
    if (connections != NULL)
    {
		while (connections != NULL)
		{
			if ((connections->local_port == port) &&
				(connections->callbacks))
			{
				error("Port %d is in use.\n", port);
				return(NULL);
			}
			//Next listener.
			connections = connections->next;
		}
	}
	
	//Allocate memory for the new connection.
	listening_connection = (struct net_connection *)db_zalloc(sizeof(struct net_connection), "listening_connection tcp_listen");
	if (!listening_connection)
	{
		error("Could not allocate memory for connection.\n");
		return(NULL);
	}
	listening_connection->conn = (struct espconn *)db_zalloc(sizeof(struct espconn), "espconn tcp_listen");
	if (!listening_connection->conn)
	{
		error("Could not allocate memory for ESP connection.\n");
		return(NULL);
	}
	listening_connection->conn->proto.tcp = (esp_tcp *)db_zalloc(sizeof(esp_tcp), "esp_tcp tcp_listen");
	if (!listening_connection->conn->proto.tcp)
	{
		error("Could not allocate memory for TCP connection.\n");
		return(NULL);
	}
	listening_connection->callbacks = (struct net_callback_funcs *)db_zalloc(sizeof(struct net_callback_funcs), "callbacks tcp_listen");
	if (!listening_connection->callbacks)
	{
		error("Could not allocate memory for TCP for call back pointers.\n");
		return(NULL);
	}
	
	listening_connection->type = NET_CT_TCP;
    
	conn = listening_connection->conn;
	callbacks = listening_connection->callbacks;
	
	//Setup TCP connection configuration.
	listening_connection->timeout = TCP_TIMEOUT;
	listening_connection->inactivity = 0;
	//TCP connection.
	listening_connection->type = NET_CT_TCP;
	conn->type = ESPCONN_TCP;
	//No state.
	conn->state = ESPCONN_NONE;
	//Set port.
	conn->proto.tcp->local_port = port;
    listening_connection->local_port = port;

	//Setup user callbacks.
	callbacks->connect_callback = connect_cb;
	callbacks->disconnect_callback = disconnect_cb;
	callbacks->write_finish_fn = write_finish_cb;
	callbacks->recv_callback = recv_cb;
	callbacks->sent_callback = sent_cb;
	
	//Register SDK callbacks.
	espconn_regist_connectcb(conn, tcp_connect_cb);
	espconn_regist_reconcb(conn, tcp_reconnect_cb);
	espconn_regist_disconcb(conn, tcp_disconnect_cb);
	espconn_regist_write_finish(conn, tcp_write_finish_cb);
	espconn_regist_recvcb(conn, tcp_recv_cb);
	espconn_regist_sentcb(conn, tcp_sent_cb);
		
	debug(" Accepting connections on port %d...", port);
	ret = espconn_accept(conn);
	debug(" Accept call return %d.\n", ret);
	if (ret == ESPCONN_OK)
	{
		debug(" Setting connection time out to infinity.");
		/* Set time out for all connections to infinity.
		 * Time outs are handled by #status_check in user_main.c.
		 * This is to have the ability to gracefully close WebSocket
		 * connections, using the correct sequence.
		 * If the SDK is allowed to handle time outs, it'll just call
		 * the reconnect (disconnect?) callback, when the connection has
		 * been closed, leaving no means to tell the WebSocket client
		 * that the connection has been closed.
		 */
		ret = espconn_regist_time(conn, 0, 0);
		debug(" Timeout call return %d.\n", ret);
	}
	if (ret != ESPCONN_OK)
	{
		error("Could not bind to port.\n");
		//Free unused memory.
		db_free(listening_connection);
		return(NULL);
	}

	net_add_connection(&tcp_listening_connections, listening_connection);
	n_tcp_listening_connections++;
	
    return(listening_connection);
}

/**
 * @brief Initialise TCP networking.
 * 
 * @return `true` on success.
 */
bool init_tcp(void)
{
    debug("TCP init.\n");
    if (tcp_connections != NULL)
    {
        error("TCP initialisation already done.\n");
        return(false);
    }
        
    //No open connections.
	n_tcp_connections = 0;
	tcp_connections = NULL;
	n_tcp_listening_connections = 0;
	tcp_listening_connections = NULL;
    
	return(true);
}

/**
 * @brief Close a listening TCP connection.
 * 
 * @param port Port to stop listening on.
 * @return True on success, false on failure.
 */
bool tcp_stop(unsigned int port)
{
	int ret;
	struct net_connection *listening_connection;
	struct net_connection *connections = tcp_listening_connections;
	
	debug("Stop listening for TCP on port %d.\n", port);
	
	listening_connection = net_find_connection_by_port(tcp_listening_connections, 80);
	
	if (!listening_connection)
	{
		warn("No listening connections.\n");
		return(false);
	}	
	
	debug("Closing TCP listening connection %p.\n", listening_connection);
	ret = espconn_delete(listening_connection->conn);
	debug(" SDK returned %d.\n", ret);
	if (connections != NULL)
	{
		//Remove connection from, the list of active connections.
		DL_LIST_UNLINK(listening_connection, connections);
		//Save any changes to the location of the first entry.
		tcp_connections = connections;
		
		debug(" Unlinked.\n");
	}
	db_free(listening_connection);
	debug(" Connection data freed.\n");

	return(true);
}

/**
 * @brief Pointer to first connection in a list of all connections.
 * 
 * @return Pointer to a struct #tcp_connection.
 */
struct net_connection *tcp_get_connections(void)
{
	return(tcp_connections);
}

/**
 * @brief Disconnect the TCP connection.
 * 
 * @param connection Connection to disconnect.
 */
void tcp_disconnect(struct net_connection *connection)
{
	int ret;
	
    debug("Disconnect (%p).\n", connection);
    debug(" espconn pointer %p.\n", connection->conn);
	connection->closing = true;
	ret = espconn_disconnect(connection->conn);
	debug(" SDK returned %d.\n", ret);
}

/** 
 * @brief Free up data structures used by a connection.
 * 
 * Free up the data pointed to by connection. espconn is expected to
 * clean up after itself.
 * @param connection Pointer to the data to free.
 */
//TODO: Merge with UDP code into net_connection_free.
void tcp_free(struct net_connection *connection)
{
	struct net_connection *connections = tcp_connections;
	
    debug("Freeing up connection (%p).\n", connection);
    /* As far as I can tell the espconn library takes care of freeing up the
     * structures it has allocated. */

    if (connection != NULL)
    {
		if (connection->user)
		{
			warn(" User data not NULL.\n");
		}
		//Remove connection from, the list of active connections.
		debug(" Unlinking.\n");
		DL_LIST_UNLINK(connection, connections);
		//Save any changes to the location of the first entry.
		tcp_connections = connections;
        
        //Free call backs if this is a listener.
        if (connection->callbacks)
        {
			debug( "Deallocating call back data.\n");
			db_free(connection->callbacks);
		}
		
		debug(" Deallocating connection data.\n");
        db_free(connection);
	    n_tcp_connections--;
	    
	    debug(" Connection deallocated.\n");
		debug(" Connections: %d.\n", n_tcp_connections);
		return;
    }
    warn("Could not deallocate connection.\n");
}

#ifdef DEBUG
void tcp_print_connection_status(void)
{
	debug("Active TCP connection(s):\n");
	net_print_connection_status(tcp_connections);
	debug("Listening TCP connection(s):\n");
	net_print_connection_status(tcp_listening_connections);
}	
#endif
