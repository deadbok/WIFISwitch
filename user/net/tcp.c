/**
 * @file tcp.c
 *
 * @brief TCP routines for the ESP8266.
 * 
 * Abstract the espconn interface and manage multiply connection.
 * 
 * Notes:
 *  - From glancing at the AT code, I have concluded that the member `reverse`
 *    in `struct espconn`, is there for the user. This has proven to be almost
 *    true, actually the only time it fails, it fails in a way that is really 
 *    neat, but I dare not abuse.
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
#include "tcp.h"

/**
 * @brief Array to look up a string from connection state.
 */
const char *state_names[] = {"ESPCONN_NONE", "ESPCONN_WAIT", "ESPCONN_LISTEN",\
                             "ESPCONN_CONNECT", "ESPCONN_WRITE", "ESPCONN_READ",\
                             "ESPCONN_CLOSE"};                    
/**
 * @brief Number of active connections.
 */
static unsigned char n_tcp_connections;
/**
 * @brief Doubly-linked list of active connections.
 */
static struct tcp_connection *tcp_connections = NULL;

//Forward declaration of callback functions.
static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_reconnect_cb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR tcp_disconnect_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_write_finish_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_recv_cb(void *arg, char *data, unsigned short length);
static void ICACHE_FLASH_ATTR tcp_sent_cb(void *arg);

/**
 * @brief Find a connection from the SDK pointer.
 * 
 * Find a connection using its remote IP and port.
 * 
 * @param conn Pointer to a `struct espconn`.
 * @param listening Includes listening connections if `true` else skip them.
 * @return Pointer to a struct #tcp_connection for conn.
 */
static struct tcp_connection ICACHE_FLASH_ATTR *find_connection(struct espconn *conn, bool listening)
{
	struct tcp_connection *connection = tcp_connections;

	if (conn == NULL)
	{
		warn("Empty parameter.\n");
		return(NULL);
	}	
	if (connection == NULL)
	{
		warn("No connections.\n");
		return(NULL);
	}

	debug(" Looking for connection for remote " IPSTR ":%d.\n", 
		  IP2STR(conn->proto.tcp->remote_ip),
		  conn->proto.tcp->remote_port);
	
    while (connection != NULL)
    {
		debug("Connection %p (%p) state \"%s\".\n", connection, connection->conn, state_names[connection->conn->state]);
		debug(" Remote address " IPSTR ":%d.\n", 
		  IP2STR(connection->remote_ip),
		  connection->remote_port);
		if ((os_memcmp(connection->remote_ip, conn->proto.tcp->remote_ip, 4) == 0) &&
			(connection->remote_port == conn->proto.tcp->remote_port))
		{
			if (listening)
			{
				//Everything is awesome!
				debug(" Connection found %p.\n", connection);
				return(connection);
			}
			else
			{
				//Only listening connections have call backs.
				if (!connection->callbacks)
				{
					debug(" Connection found %p.\n", connection);
					return(connection);
				}
			}
		}	
		connection = connection->next;
	}
	warn("Connection not found.\n");
	return(NULL);
}

/**
 * @brief Find a listening connection from the SDK pointer.
 * 
 * Find a connection using its local port.
 * 
 * @param conn Pointer to a `struct espconn`.
 * @return Pointer to a struct #tcp_connection for conn.
 */
static struct tcp_connection ICACHE_FLASH_ATTR *find_listening_connection(unsigned int port)
{
	struct tcp_connection *connection = tcp_connections;

	if (connection == NULL)
	{
		warn("No connections.\n");
		return(NULL);
	}

	debug(" Looking for listening connection on port %d.\n", port);

    while (connection != NULL)
    {
		debug("Connection %p (%p) state \"%s\".\n", connection, connection->conn, state_names[connection->conn->state]);
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
static void ICACHE_FLASH_ATTR add_active_connection(struct tcp_connection *connection)
{
	struct tcp_connection *connections = tcp_connections;
	
	debug("Adding %p to active connections.\n", connection);
	//Add the connection to the list of active connections.
	if (connections == NULL)
	{
		tcp_connections = connection;
		connection->prev = NULL;
		connection->next = NULL;
	}
	else
	{
		DL_LIST_ADD_END(connection, connections);
	}
	n_tcp_connections++;
	debug(" Connections: %d.\n", n_tcp_connections);
}

/**
 * @brief Internal function to print espconn status.
 * 
 * @param status Status to print.
 */
static void ICACHE_FLASH_ATTR print_status(const int status)
{
    switch(status)
    {
        case ESPCONN_OK:
            debug("OK.\n");
            break;
        case ESPCONN_MEM:
            error("Out of memory.\n");
            break;
        case ESPCONN_TIMEOUT:
            warn("Timeout.\n");
            break;            
        case ESPCONN_RTE:
            error("Routing problem.\n");
            break;
        case ESPCONN_INPROGRESS:
            db_printf("Operation in progress...\n");
            break;
        case ESPCONN_ABRT:
            warn("Connection aborted.\n");
            break;
        case ESPCONN_RST:
            warn("Connection reset.\n");
            break;
        case ESPCONN_CLSD:
            warn("Connection closed.\n");
            break;
        case ESPCONN_CONN:
            os_printf("Not connected.\n");
            break;
        case ESPCONN_ARG:
            os_printf("Illegal argument.\n");
            break;
        case ESPCONN_ISCONN:
            warn("Already connected.\n");
            break;
        default:
            warn("Unknown status: %d\n", status);
            break;    
    }
}

/**
 * @brief Internal callback for when a new connection has been made.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg)
{
	struct espconn *conn = arg;
    struct tcp_connection *connection;
    struct tcp_connection *listening_connection;
    
    debug("TCP connected (%p).\n", conn);
    
    listening_connection = find_listening_connection(conn->proto.tcp->local_port);
    
	connection = (struct tcp_connection *)db_zalloc(sizeof(struct tcp_connection), "connection tcp_connect_cb");
	connection->conn = arg;
	
	//Register SDK callbacks.
	espconn_regist_connectcb(connection->conn, tcp_connect_cb);
	espconn_regist_reconcb(connection->conn, tcp_reconnect_cb);
	espconn_regist_disconcb(connection->conn, tcp_disconnect_cb);
	espconn_regist_write_finish(connection->conn, tcp_write_finish_cb);
	espconn_regist_recvcb(connection->conn, tcp_recv_cb);
	espconn_regist_sentcb(connection->conn, tcp_sent_cb);

	//Nothing's happening.
	connection->sending = false;
	connection->closing = false;
	
	//Save connection addresses.
	os_memcpy(connection->local_ip, conn->proto.tcp->local_ip, 4);
	connection->local_port = conn->proto.tcp->local_port;
	os_memcpy(connection->remote_ip, conn->proto.tcp->remote_ip, 4);
	connection->remote_port = conn->proto.tcp->remote_port;
	
	debug(" Remote address " IPSTR ":%d.\n", 
		  IP2STR(connection->remote_ip),
		  connection->remote_port);

	add_active_connection(connection);
	
	if (listening_connection)
	{
		//Call callback.
		if (listening_connection->callbacks->connect_callback != NULL)
		{
			listening_connection->callbacks->connect_callback(connection);
		}
	}
	else
	{
		warn("Could not find listening connection.\n");
	}
	debug("Leaving TCP connect call back (%p).\n", conn);
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
static void ICACHE_FLASH_ATTR tcp_reconnect_cb(void *arg, sint8 err)
{
    struct espconn *conn = arg;
    struct tcp_connection *connection;
    struct tcp_connection *listening_connection;
            
    debug("TCP reconnected (%p).\n", conn);
    print_status(err);
    
    connection = find_connection(conn, false);
    listening_connection = find_listening_connection(conn->proto.tcp->local_port);
    
	if (connection)
	{
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
		//Set new data
		connection->callback_data.arg = arg;
		
		debug("Handling as disconnect.\n");

		if (listening_connection)
		{
			debug(" Listener found %p.\n", listening_connection);
			//Call callback.
			if (listening_connection->callbacks->disconnect_callback != NULL)
			{
				listening_connection->callbacks->disconnect_callback(connection);
			}
		}
		else
		{
			debug(" No one is listening.\n");
		}
		tcp_free(connection);
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
static void ICACHE_FLASH_ATTR tcp_disconnect_cb(void *arg)
{
    struct espconn *conn = arg;
    struct tcp_connection *connection;
    struct tcp_connection *listening_connection;
            
    debug("TCP disconnected (%p).\n", arg);
    
    connection = find_connection(conn, false);
    listening_connection = find_listening_connection(conn->proto.tcp->local_port);
    
	if (connection)
	{
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
		//Set new data
		connection->callback_data.arg = arg;

		if (listening_connection)
		{
			debug(" Listener found %p.\n", listening_connection);
			//Call callback.
			if (listening_connection->callbacks->disconnect_callback != NULL)
			{
				listening_connection->callbacks->disconnect_callback(connection);
			}
		}
		else
		{
			debug(" No one is listening.\n");
		}
		tcp_free(connection);
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
static void ICACHE_FLASH_ATTR tcp_write_finish_cb(void *arg)
{
    struct espconn *conn = arg;
    struct tcp_connection *connection;
    struct tcp_connection *listening_connection;
    
    debug("TCP write done (%p).\n", arg);
    
    connection = find_connection(conn, false);
    listening_connection = find_listening_connection(conn->proto.tcp->local_port);
    
	if (connection)
	{
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
		//Set new data
		connection->callback_data.arg = arg;

		if (listening_connection)
		{
			debug(" Listener found %p.\n", listening_connection);
			//Call callback.
			if (listening_connection->callbacks->write_finish_fn != NULL)
			{
				listening_connection->callbacks->write_finish_fn(connection);
			}
			debug("Leaving TCP write finish call back (%p).\n", conn);
			return;
		}
		debug(" No one is listening.\n");
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
static void ICACHE_FLASH_ATTR tcp_recv_cb(void *arg, char *data, unsigned short length)
{
    struct espconn  *conn = arg;
    struct tcp_connection *connection;
    struct tcp_connection *listening_connection;
    
    debug("TCP received (%p).\n", arg);
    debug("%s\n", data);
    
    connection = find_connection(conn, false);
    listening_connection = find_listening_connection(conn->proto.tcp->local_port);
    
	if (connection)
	{
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
		//Set new data
		connection->callback_data.arg = arg;
		connection->callback_data.data = data;
		connection->callback_data.length = length;

		if (listening_connection)
		{
			debug(" Listener found %p.\n", listening_connection);
			//Call callback.
			if (listening_connection->callbacks->recv_callback != NULL)
			{
				listening_connection->callbacks->recv_callback(connection);
			}
			debug("Leaving TCP receive call back (%p).\n", conn);
			return;
		}
		debug(" No one is listening.\n");
		debug("Leaving TCP receive call back (%p).\n", conn);
		return;
	}
	warn("Could not find receiving connection.\n");
	debug("Leaving TCP receive call back (%p).\n", conn);
}

/**
 * @brief Internal callback, called when TCP data has been sent.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void ICACHE_FLASH_ATTR tcp_sent_cb(void *arg)
{
    struct espconn *conn = arg;
    struct tcp_connection *connection;
    struct tcp_connection *listening_connection;
    
    debug("TCP sent (%p).\n", conn);
    
    connection = find_connection(conn, false);
    listening_connection = find_listening_connection(conn->proto.tcp->local_port);

	if (connection)
	{
		connection->sending = false;
		//Clear previous data.
		os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
		//Set new data
		connection->callback_data.arg = arg;

		if (listening_connection)
		{
			debug(" Listener found %p.\n", listening_connection);
			//Call callback.
			if (listening_connection->callbacks->sent_callback != NULL)
			{
				listening_connection->callbacks->sent_callback(connection);
			}
			debug("Leaving TCP sent call back (%p).\n", conn);
			return;
		}
		debug(" No one is listening.\n");
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
 * @return `true` on success.
 */
bool ICACHE_FLASH_ATTR tcp_listen(unsigned int port,
								  tcp_callback connect_cb, 
								  tcp_callback disconnect_cb,
								  tcp_callback write_finish_cb,
								  tcp_callback recv_cb,
								  tcp_callback sent_cb)
{
    int ret;
    struct espconn *conn;
    struct tcp_callback_funcs *callbacks;
	struct tcp_connection *connections = tcp_connections;
	struct tcp_connection *listening_connection = NULL;
    
    debug("Adding TCP listener on port %d.\n", port);
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
	listening_connection = (struct tcp_connection *)db_zalloc(sizeof(struct tcp_connection), "listening_connection tcp_listen");
	if (!listening_connection)
	{
		error("Could not allocate memory for connection.\n");
		return(false);
	}
	listening_connection->conn = (struct espconn *)db_zalloc(sizeof(struct espconn), "espconn tcp_listen");
	if (!listening_connection->conn)
	{
		error("Could not allocate memory for ESP connection.\n");
		return(false);
	}
	listening_connection->conn->proto.tcp = (esp_tcp *)db_zalloc(sizeof(esp_tcp), "esp_tcp tcp_listen");
	if (!listening_connection->conn->proto.tcp)
	{
		error("Could not allocate memory for TCP connection.\n");
		return(false);
	}
	listening_connection->callbacks = (struct tcp_callback_funcs *)db_zalloc(sizeof(struct tcp_callback_funcs), "callbacks tcp_listen");
	if (!listening_connection->callbacks)
	{
		error("Could not allocate memory for TCP for call back pointers.\n");
		return(false);
	}
    
	conn = listening_connection->conn;
	callbacks = listening_connection->callbacks;
	
	//Setup TCP connection configuration.
	//TCP connection.
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
#ifdef DEBUG
	print_status(ret);
#endif
	if (ret == ESPCONN_OK)
	{
		debug(" Setting connection time out to 60 secs...");
		//Set time out for all connections.
		ret = espconn_regist_time(conn, 60, 0);
	#ifdef DEBUG
		print_status(ret);
	#endif
	}
	if (ret != ESPCONN_OK)
	{
		//Free unused memory.
		db_free(listening_connection);
		return(false);
	}

	add_active_connection(listening_connection);
    return(true);
}

/**
 * @brief Initialise TCP networking.
 * 
 * @return `true` on success.
 */
bool ICACHE_FLASH_ATTR init_tcp(void)
{
    debug("TCP init.\n");
    if (tcp_connections != NULL)
    {
        error("TCP init already done.\n");
        return(false);
    }
        
    //No open connections.
    n_tcp_connections = 0;
    tcp_connections = NULL;
    
	return(true);
}

/**
 * @brief Close a listening TCP connection.
 * 
 * @param port Port to stop listening on.
 * @return True on success, false on failure.
 */
bool ICACHE_FLASH_ATTR tcp_stop(unsigned int port)
{
	int ret;
	struct tcp_connection *listening_connection;
	struct tcp_connection *connections = tcp_connections;
	
	debug("Stop listening for TCP on port %d.\n", port);
	
	listening_connection = find_listening_connection(80);
	
	if (!listening_connection)
	{
		warn("No listening connections.\n");
		return(false);
	}	
	
		debug("Closing TCP listening connection %p.\n", listening_connection);
		ret = espconn_delete(listening_connection->conn);
#ifdef DEBUG
		print_status(ret);
#endif
		if (connections != NULL)
		{
			//Remove connection from, the list of active connections.
			DL_LIST_UNLINK(listening_connection, connections);
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
struct tcp_connection ICACHE_FLASH_ATTR *tcp_get_connections(void)
{
	return(tcp_connections);
}

/** 
 * @brief Send TCP data.
 * 
 * @param connection Connection used for sending the data.
 * @param data Pointer to the data to send.
 * @param size Length of data in bytes.
 * 
 * @return true on success, false otherwise.
 */
bool ICACHE_FLASH_ATTR tcp_send(struct tcp_connection *connection, char *data, size_t size)
{
    debug("Sending %d bytes of TCP data (%p using %p),\n", size, data, connection);
	debug(" espconn pointer %p.\n", connection->conn);
	
	if (connection->sending)
	{
		error(" Still sending something else.\n");
		return(false);
	}    
#ifdef DEBUG
	uart0_tx_buffer((unsigned char *)data, size);
#endif //DEBUG
	if (connection->conn)
	{
		connection->sending = true;
		return(espconn_sent(connection->conn, (unsigned char*)data, size));
	}
	warn(" Connection is empty.\n");
	return(false);
}

/**
 * @brief Disconnect the TCP connection.
 * 
 * @param connection Connection to disconnect.
 */
void ICACHE_FLASH_ATTR tcp_disconnect(struct tcp_connection *connection)
{
	int ret;
	
    debug("Disconnect (%p).\n", connection);
    debug(" espconn pointer %p.\n", connection->conn);
	connection->closing = true;
	ret = espconn_disconnect(connection->conn);
#ifdef DEBUG
	print_status(ret);
#endif
}

/** 
 * @brief Free up data structures used by a connection.
 * 
 * Free up the data pointed to by connection. espconn is expected to
 * clean up after itself.
 * @param connection Pointer to the data to free.
 */
void ICACHE_FLASH_ATTR tcp_free(struct tcp_connection *connection)
{
	struct tcp_connection *connections = tcp_connections;
	
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
	    n_tcp_connections--;
		debug(" Connections: %d.\n", n_tcp_connections);
		return;
    }
    warn("Could not deallocate connection.\n");
}
