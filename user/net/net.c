/** @file net.c
 *
 * @brief General network related variables and functions.
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
#include "net/net.h"
#include "tools/ring.h"
#include "user_config.h"
#include "driver/uart.h"
#include "net/net-task.h"
#include "debug.h"

const char *state_names[] = {"ESPCONN_NONE", "ESPCONN_WAIT", "ESPCONN_LISTEN",\
                             "ESPCONN_CONNECT", "ESPCONN_WRITE", "ESPCONN_READ",\
                             "ESPCONN_CLOSE"};
const char *net_ct_names[] = {"None", "TCP", "HTTP", "WebSocket", "UDP", "DNS"};

/**
 * @brief Pointer to the buffer being send or NULL.
 */
static char *net_sending = NULL;

/**
 * @brief Info for an item waiting to be send.
 */
struct net_send_queue_item
{
	/**
	 * @brief The data waiting to be send.
	 */
	char *data;
	/**
	 * @brief Size of the data waiting.
	 */
	size_t len;
	/**
	 * @brief Connection to use when sending.
	 */ 
	struct espconn *connection;
};

/**
 * @brief Send buffer.
 */
static struct ring_buffer *net_send_queue;

void init_net(void)
{
	net_register_tasks();
}

/**
 * @brief Send something over WIFI using the SDK API.
 */
static bool net_sdk_send(struct espconn *connection, char *data, size_t size)
{
	signed char ret;
	
    debug("Sending %d bytes of TCP data (from %p using %p),\n", size, data, connection);
	if (!connection)
	{
		error("No connection to use when sending.\n");
		return(false);
	}
	
	if (net_sending)
	{
		error(" Sending something else.\n");
		return(false);
	}    
#ifdef DEBUG
	uart0_tx_buffer((unsigned char *)data, size);
#endif //DEBUG
	net_sending = data;
	ret = espconn_send(connection, (unsigned char*)data, size);
	debug(" Send status %d: ", ret);
	if (!ret)
	{
		return(true);
	}
	debug(" Send returned an error status.\n");
	return(false);
}

/**
 * @todo Think about passing a net_connection struct instead of espconn struct.
 */
size_t net_send(char *data, size_t len, struct espconn *connection)
{
	char *buffer = NULL;
	struct net_send_queue_item *queue_item = NULL;
	
	/**
	 * @todo Maybe move the buffer pointer to the connection struct,
	 * 	     And add the connection struct to the ring buffer.
	 */
	debug("Sending %d bytes using %p.\n", len, connection);
	
	/**
	 * @todo Check validity of parameters.
	 */
	if (!net_send_queue)
	{
		debug("Creating send buffer.\n");
		net_send_queue = db_malloc(sizeof(struct ring_buffer), "net_send net_send_queue");
		init_ring(net_send_queue, sizeof(struct net_send_queue_item), NET_MAX_SEND_QUEUE);
	}
	
	if (net_sending)
	{
		debug("Already sending, adding to queue.\n");
		
		buffer = db_malloc(len, "net_send buffer");
		os_memcpy(buffer, data, len);
		/**
		 * @todo Check return value of ring_push_back.
		 */
		queue_item = ring_push_back(net_send_queue);
		
		queue_item->data = buffer;
		queue_item->len = len;
		queue_item->connection = connection;
		return(len);
	}
	
	debug("Sending data.\n");
	if (net_sdk_send(connection, data, len))
	{
		return(len);
	}
	error("Send failed.\n");
	return(0);
}

void net_sent_callback(void)
{
	struct net_send_queue_item *queue_item = NULL;
	
	debug("Processing buffered send requests.\n");
	//db_free(net_sending);
	net_sending = NULL;
	if (!net_send_queue)
	{
		//Nothing to do.
		return;
	}
	
	//Get next waiting item.
	queue_item = ring_pop_front(net_send_queue);
	if (queue_item)
	{
		debug("Sending buffered data.\n");
		if (!net_sdk_send(queue_item->connection, queue_item->data, queue_item->len))
		{
			error("Send failed.\n");
		}
		db_free(queue_item);
	}
}

/**
 * @brief Disconnect the TCP connection.
 * 
 * @param connection Connection to disconnect.
 * 
 * From SDK docs:
 *		 **Do not call this API in any espconn callback. If needed,
 * 		 please use system_os_task and system_os_post to trigger
 * 		 espconn_disconnect.**
 */
void net_disconnect(struct net_connection *connection)
{
	int ret;
	
    debug("Disconnect (%p).\n", connection);
    debug(" espconn pointer %p.\n", connection->conn);
	connection->closing = true;
	/**
	 * @todo From SDK docs:
	 *		 Do not call this API in any espconn callback. If needed,
	 * 		 please use system_os_task and system_os_post to trigger
	 * 		 espconn_disconnect.
	 */

	ret = espconn_disconnect(connection->conn);
	if (ret)
	{
		warn("Could not disconnect %p.\n", connection);
	}
	debug(" SDK returned %d.\n", ret);
}


/* This is my quirky way of making the variable RO.
 */
bool net_is_sending(void)
{
	return((bool)net_sending);
}

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
struct net_connection *net_find_connection(struct net_connection *connections, struct espconn *conn)
{
	struct net_connection *connection = connections;

	if (conn == NULL)
	{
		warn("Empty parameter.\n");
		return(NULL);
	}
	debug(" Looking for connection for remote " IPSTR ":%d.\n", 
		  IP2STR(conn->proto.tcp->remote_ip),
		  conn->proto.tcp->remote_port);

	if (connection == NULL)
	{
		warn("No connections.\n");
		return(NULL);
	}
	
    while (connection != NULL)
    {
		debug("Connection %p (%p) state \"%s\".\n", connection, connection->conn, state_names[connection->conn->state]);
		debug(" Remote address " IPSTR ":%d.\n", 
		  IP2STR(connection->remote_ip),
		  connection->remote_port);
		if ((os_memcmp(connection->remote_ip, conn->proto.tcp->remote_ip, 4) == 0) &&
			(connection->remote_port == conn->proto.tcp->remote_port))
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
 * @brief Find a connection using its local port.
 *
 * @param connections Pointer to a list of `struct net_connection`'s.
 * @param conn Pointer to a `struct espconn`.
 * @return Pointer to a struct #tcp_connection for conn.
 */
struct net_connection *net_find_connection_by_port(struct net_connection *connections, unsigned int port)
{
	struct net_connection *connection = connections;

	if (connection == NULL)
	{
		warn("No connections.\n");
		return(NULL);
	}

	debug(" Looking for connection on port %d.\n", port);
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
 * @brief Add a connection to a list of connections.
 *
 * @param connections Pointer to a list of `struct net_connection`'s.
 * @param connection Pointer to the connection to add.
 */
void net_add_connection(struct net_connection **connections, struct net_connection *connection)
{
	struct net_connection *temp_connections = *connections;
	
	debug("Adding %p to connections to %p.\n", connection, connections);
	//Add the connection to the list of active connections.
	if (*connections == NULL)
	{
		*connections = connection;
		connection->prev = NULL;
		connection->next = NULL;
	}
	else
	{
		DL_LIST_ADD_END(connection, temp_connections);
	}
}

#ifdef DEBUG
void net_print_connection_status(struct net_connection *connections)
{
	struct net_connection *connection = connections;
	unsigned int n_connections = 0;
	
	if (connections)
	{
		debug("Listing connections from %p.\n", connections);	
	}
	while (connection)
	{
		n_connections++;
		if (connection->conn)
		{
			if (connection->type < N_NET_CT)
			{
				debug("%s connection", net_ct_names[connection->type]);
			}
			else
			{
				debug("Unknown connection");
			}
			if (connection->conn->state <= ESPCONN_CLOSE)
			{
				debug(" %p (%p) state \"%s\".\n",
					  connection, connection->conn,
					  state_names[connection->conn->state]);
			}
			else
			{
				debug(" %p (%p) state unknown (%d).\n",
					  connection, connection->conn,
					  connection->conn->state);
			}
			
			debug(" Remote address " IPSTR ":%d.\n", 
				  IP2STR(connection->remote_ip),
				  connection->remote_port);

			if (connection->conn->state < ESPCONN_CLOSE)
			{
				debug(" SDK remote address " IPSTR ":%d.\n", 
					  IP2STR(connection->conn->proto.tcp->remote_ip),
					  connection->conn->proto.tcp->remote_port);
			}
		}
		else
		{
			if (connection->type < N_NET_CT)
			{
				debug("%s connection", net_ct_names[connection->type]);
			}
			else
			{
				debug("Unknown connection");
			}
			debug(" %p, no SDK connections.\n",
				  connection);
			debug(" Remote address " IPSTR ":%d.\n", 
				  IP2STR(connection->remote_ip),
				  connection->remote_port);
		}
		connection = connection->next;
	}
	if (connections)
	{	
		debug("%d connection(s).\n", n_connections);
	}
}
#endif
