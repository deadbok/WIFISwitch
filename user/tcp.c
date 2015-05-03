/* tcp.c
 *
 * TCP Routines for the ESP8266.
 * 
 * Abstract the espconn interface and manage multiply connection.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
 * 
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
#include "ip_addr.h"
#include "mem.h"
#include "user_config.h"
#include "tcp.h"

#define TCP_MAX_CONNECTIONS 10
//Active connections.
static unsigned char n_tcp_connections;
struct tcp_connection *tcp_connections;

//Listening connection
struct tcp_connection *listening_connection;

//Internal function to print espconn errors.
static void ICACHE_FLASH_ATTR print_status(int status)
{
    switch(status)
    {
        case ESPCONN_OK:
            debug("OK.\n");
            break;
        case ESPCONN_MEM:
            os_printf("Out of memory.\n");
            break;
        case ESPCONN_TIMEOUT:
            os_printf("Timeout.\n");
            break;            
        case ESPCONN_RTE:
            os_printf("Routing problem.\n");
            break;
        case ESPCONN_INPROGRESS:
            os_printf("Operation in progress...\n");
            break;
        case ESPCONN_ABRT:
            os_printf("Connection aborted.\n");
            break;
        case ESPCONN_RST:
            os_printf("Connection reset.\n");
            break;
        case ESPCONN_CLSD:
            os_printf("Connection closed.\n");
            break;
        case ESPCONN_CONN:
            os_printf("Not connected.\n");
            break;
        case ESPCONN_ARG:
            os_printf("Illegal argument.\n");
            break;
        case ESPCONN_ISCONN:
            os_printf("Already connected.\n");
            break;
        default:
            os_printf("Unknown status: %d\n", status);
            break;    
    }
}

//Find connection data from a pointer to struct espconn.
static struct tcp_connection ICACHE_FLASH_ATTR *tcp_find_conn(void *arg)
{
	struct tcp_connection   *connections = tcp_connections;
    
    debug("TCP finding connection %p\n", arg);

	do
    {
        debug(" Trying %p, %p\n", connections, connections->conn);
		if (connections->conn == arg)
        {
            debug(" Found %p\n", connections);
            return(connections);
        }
        
        if (connections->next != NULL)
        {
            connections = connections->next;
        }
	} while (connections->next != NULL);
	os_printf("ERROR: Could not find connection for: %p\n", arg);
	return NULL;
}

static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg)
{
    struct tcp_connection   *connection;
    
    //Check if there are any free connections.
    if (n_tcp_connections < TCP_MAX_CONNECTIONS)
    {
        connection = (struct tcp_connection *)os_zalloc(sizeof(struct tcp_connection));
        connection->conn = arg;
        
        debug("TCP connected (%p).\n", connection);
        debug("  espconn data: %p.\n", connection->conn);

        //Add connection to the list.
        if (tcp_connections == NULL)
        {
            debug(" First connection.\n");
            tcp_connections = connection;
            connection->prev = NULL;
            connection->next = NULL;
        }
        else
        {
            debug(" Connection number %d.\n", n_tcp_connections);
            DL_LIST_ADD_END(connection, tcp_connections);
        }
        //Increase number of connections.        
        n_tcp_connections++;
        //Call callback.
        listening_connection->callbacks.connect_callback(connection);
    }
    else
    {
        os_printf("No more free TCP connections.\n");
        return;
    }
}

static void ICACHE_FLASH_ATTR tcp_reconnect_cb(void *arg, sint8 err)
{
    struct tcp_connection   *connection;
    
    connection = tcp_find_conn(arg);
    
    debug("TCP resconnected (%p).\n", connection);
    
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    connection->callback_data.err = err;
    //Call call back.
    listening_connection->callbacks.reconnect_callback(connection);
}

static void ICACHE_FLASH_ATTR tcp_disconnect_cb(void *arg)
{
    
    debug("TCP disconnected (%p).\n", arg);
    //~ 
    //~ //Clear previous data.
    //~ os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //~ //Set new data
    //~ connection->callback_data.arg = arg;
    //~ //Call call back.
    //~ listening_connection->callbacks.disconnect_callback(connection);
}

static void ICACHE_FLASH_ATTR tcp_write_finish_cb(void *arg)
{
    struct tcp_connection   *connection;
    
    connection = tcp_find_conn(arg);
    
    debug("TCP write done (%p).\n", connection);
    
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    //Call call back.
    listening_connection->callbacks.write_finish_fn(connection);
}

static void ICACHE_FLASH_ATTR tcp_recv_cb(void *arg, char *data, unsigned short length)
{
    struct tcp_connection   *connection;
    
    connection = tcp_find_conn(arg);
    
    debug("TCP received (%p).\n", connection);
    debug("%s\n", data);
    
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    connection->callback_data.data = data;
    connection->callback_data.length = length;
    //Call call back.
    listening_connection->callbacks.recv_callback(connection);
}

static void ICACHE_FLASH_ATTR tcp_sent_cb(void *arg)
{
    struct tcp_connection   *connection;
    
    connection = tcp_find_conn(arg);
    
    debug("TCP sent (%p).\n", connection);
    
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    //Call call back.
    listening_connection->callbacks.sent_callback(connection);
}

/* Send TCP data to the current connection.
 * 
 * Parameters:
 *  data
 *   Pointer to the data to send
 * 
 * Returns:
 *  espconn error status
 */
char ICACHE_FLASH_ATTR tcp_send(struct tcp_connection *connection, char *data)
{
    debug("Sending TCP: %s\n", data);
    return(espconn_sent(connection->conn, (unsigned char *)data, os_strlen(data)));
}

/* Disconnect the current TCP connection.
 * 
 * Parameters:
 *  none
 * 
 * Returns:
 *  espconn error status
 */
char ICACHE_FLASH_ATTR tcp_disconnect(struct tcp_connection *connection)
{
    int ret;
    
    debug("Disconnecting (%p)...", connection);
    //Disconnect
    ret = espconn_disconnect(connection->conn);
#ifdef DEBUG
    print_status(ret);
#endif
    //Unlink connection from list
    DL_LIST_UNLINK(connection);
    //Free up connection data
    //connection->conn is left for the sdk to handle.
    debug(" Freeing memory.\n");
    os_free(connection);
    n_tcp_connections--;
    debug(" Open connections: %d\n", n_tcp_connections);
    if (n_tcp_connections == 0)
    {
        tcp_connections = NULL;
    }
    
    return(ret);
}

/* Free up data structures used by a connection.
 * 
 * Parameters:
 *  connection
 *   Pointer to the connection data.
 * 
 * Returns:
 *  none
 */
void ICACHE_FLASH_ATTR tcp_free(struct tcp_connection *connection)
{
    //Unlink connection from list
    DL_LIST_UNLINK(connection);
    //Free up connection data
    os_free(connection->conn->proto.tcp);
    os_free(connection->conn);
    os_free(connection);
}

/* Open a TCP connection.
 * 
 * Parameters:
 *  port
 *   The TCP port to bind to.
 * 
 * Returns:
 *  none
 */
struct tcp_connection ICACHE_FLASH_ATTR *tcp_listen(int port, tcp_callback connect_cb, 
                                tcp_callback reconnect_cb, tcp_callback disconnect_cb,
                                tcp_callback write_finish_cb, tcp_callback recv_cb,
                                tcp_callback sent_cb)
{
    int                         ret;
    struct tcp_connection       *connection;
    struct espconn              *conn;
    struct tcp_callback_funcs   *callbacks;
    
    debug("TCP listen.\n");
    //Bail out if there somebody is already listening.
    if (listening_connection == NULL)
    {
        //Allocate memory for the new connection.
        connection = (struct tcp_connection *)os_zalloc(sizeof(struct tcp_connection));
        connection->conn = (struct espconn *)os_zalloc(sizeof(struct espconn));
        debug(" Allocated memory for connection %p.\n", connection);
        debug("  espconn data: %p.\n", &connection->conn);
        debug("  Callbacks: %p.\n", &connection->callbacks);
        debug("  Callback data: %p.\n", &connection->callback_data);
        conn = connection->conn;
        callbacks = &connection->callbacks;
        
        //Setup TCP connection configuration.
        //TCP connection.
        conn->type = ESPCONN_TCP;
        //No state.
        conn->state = ESPCONN_NONE;
        //TCP setup.
        conn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
        //Set port.
        conn->proto.tcp->local_port = port;
        //Setup internal callbacks.
        conn->proto.tcp->connect_callback = tcp_connect_cb;
        conn->proto.tcp->reconnect_callback = tcp_reconnect_cb;
        conn->proto.tcp->disconnect_callback = tcp_disconnect_cb;
        conn->proto.tcp->write_finish_fn = tcp_write_finish_cb;
        conn->recv_callback = tcp_recv_cb;
        conn->sent_callback = tcp_sent_cb;
        
        //Setup user callbacks.
        callbacks->connect_callback = connect_cb;
        callbacks->reconnect_callback = reconnect_cb;
        callbacks->disconnect_callback = disconnect_cb;
        callbacks->write_finish_fn = write_finish_cb;
        callbacks->recv_callback = recv_cb;
        callbacks->sent_callback = sent_cb;
        
        debug(" Accepting connections on port %d...", port);
        ret = espconn_accept(conn);
#ifdef DEBUG
        print_status(ret);
#endif
        listening_connection = connection;
    }
    else
    {
        os_printf("ERROR: Only one listening TCP connection supported.\n");
        return(NULL);
    }
    return(connection);
}

/* Initialise TCP networking.
 * 
 * Parameters:
 * 
 * Returns:
 *  none
 */
void ICACHE_FLASH_ATTR init_tcp(void)
{
    debug("TCP init.\n");
    if (tcp_connections != NULL)
    {
        debug("TCP init already called once.n");
        //TODO: Close and deallocate connection.
    }
    n_tcp_connections = 0;
    tcp_connections = NULL;

    //Listening connection
    listening_connection = NULL;
    
    n_tcp_connections = 0;
}
