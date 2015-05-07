/** tcp.c
 *
 * TCP Routines for the ESP8266.
 * 
 * Abstract the espconn interface and manage multiply connection.
 * 
 * NOTES:
 *  - From glancing at the AT code, I have concluded that the member `reverse`
 *    in `struct espconn`, is there to be abused.
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

//Active connections.
static unsigned char n_tcp_connections;
struct tcp_connection *tcp_connections = NULL;

//Listening connection
static struct tcp_connection *listening_connection;

//Forward declaration of callback functions.
static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_reconnect_cb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR tcp_disconnect_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_write_finish_cb(void *arg);

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

/** Free up data structures used by a connection.
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
    debug("Freeing up connection (%p).\n", connection);
    /* As far as I can tell the espconn library takes care of freeing up the
     * structures it has allocated. */

    //Remove connection from, the list of active connections.
    DL_LIST_UNLINK(connection);
    debug(" Unlinked.\n");
    
    if (connection != NULL)
    {
        os_free(connection);
        debug("Freed.\n");
    }
    n_tcp_connections--;
    //Emtpy list, if this was the last connection.
    if (n_tcp_connections == 0)
    {
        tcp_connections = NULL;
    }
    debug(" Memory deallocated.\n");
}

static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg)
{
    struct tcp_connection   *connection;
    
    //Check if there are any free connections.
    if (n_tcp_connections < TCP_MAX_CONNECTIONS)
    {
        connection = (struct tcp_connection *)os_zalloc(sizeof(struct tcp_connection));
        connection->conn = arg;
        
        //Set internal callbacks.
        connection->conn->proto.tcp->connect_callback = tcp_connect_cb;
        connection->conn->proto.tcp->reconnect_callback = tcp_reconnect_cb;
        connection->conn->proto.tcp->disconnect_callback = tcp_disconnect_cb;
        connection->conn->proto.tcp->write_finish_fn = tcp_write_finish_cb;
        
        //Set user callbacks.
        os_memcpy(&connection->callbacks, &listening_connection->callbacks, 
                  sizeof(struct tcp_callback_funcs));
        //Save our connection data
        connection->conn->reverse = connection;
        
        debug("TCP connected (%p).\n", connection);

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
            debug(" Connection number %d.\n", n_tcp_connections + 1);
            struct tcp_connection  *connections = tcp_connections;
            
            DL_LIST_ADD_END(connection, connections);
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
    struct espconn *conn = arg;
    struct tcp_connection   *connection = conn->reverse;
        
    debug("TCP reconnected (%p).\n", connection);
    
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
    /* espconn puts a pointer to the LISTENING connection in `arg`,
     * I would have expected a pointer to the closing connection.
     *
     * The wabbit hole goes deeper! Even though the callback seems to get a
     * pointer to the listening connection, it also has a pointer to my internal
     * connection structure in the `reverse` member. Even though I have set the
     * `reverse` pointer to NULL (twice), when I created the struct, when it gets
     * to the disconnect callback it seems to contain a pointer to the 
     * struct tcp_connection of the closing connection. As far as I have been
     * able to debug this, it seems that up until the very point that the
     * connection is closed, the pointer keeps its initial value. This is great,
     * but since I have no reason to think that the Espressif guys, are copying 
     * pointers to my data structures, into a convenient place on purpose, I can
     * not rely on this behavior, and have to do things the hard way, and look 
     * up the closing connection.*/
    struct tcp_connection *connection = tcp_connections;
    
    debug("TCP disconnected.\n");
    
    debug(" Finding connection: ");
    while (connection != NULL)
    {
        if (connection->conn->state == ESPCONN_CLOSE)
        {
            debug("%p.\n", connection);
            break;
        }
        connection = connection->next;
    }
    
    if (connection == NULL)
    {
        os_printf("ERROR: Could not find closing connection.\n");
        return;
    }
 
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    //Call call back.
    listening_connection->callbacks.disconnect_callback(connection);
    //Free the connection
    tcp_free(connection);
}

static void ICACHE_FLASH_ATTR tcp_write_finish_cb(void *arg)
{
    struct espconn *conn = arg;
    struct tcp_connection   *connection = conn->reverse;
    
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
    struct espconn  *conn = arg;
    struct tcp_connection   *connection = conn->reverse;
    
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
    struct espconn *conn = arg;
    struct tcp_connection   *connection = conn->reverse;
    
    debug("TCP sent (%p).\n", connection);
    
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    //Call call back.
    listening_connection->callbacks.sent_callback(connection);
}

/** Send TCP data to the current connection.
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

/**Disconnect the current TCP connection.
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
    tcp_free(connection);
    debug(" Open connections: %d\n", n_tcp_connections);
    if (n_tcp_connections == 0)
    {
        tcp_connections = NULL;
    }
    
    return(ret);
}

/**Open a TCP connection.
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
        connection->conn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
        //Save our connection data
        connection->conn->reverse = NULL;
        
        debug(" Allocated memory for connection %p.\n", connection);
        debug("  espconn data: %p.\n", connection->conn);
        debug("  TCP data: %p.\n", connection->conn->proto.tcp);
        debug("  Callbacks: %p.\n", &connection->callbacks);
        debug("  Callback data: %p.\n", &connection->callback_data);
        debug("  Reverse pointer to connection: %p.\n", connection->conn->reverse);
        
        conn = connection->conn;
        callbacks = &connection->callbacks;
        
        //Setup TCP connection configuration.
        //TCP connection.
        conn->type = ESPCONN_TCP;
        //No state.
        conn->state = ESPCONN_NONE;
        //TCP setup.
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

/**Initialise TCP networking.
 * 
 * Parameters:
 * 
 * Returns:
 *  none
 */
void ICACHE_FLASH_ATTR init_tcp(void)
{
    int ret;
    
    debug("TCP init.\n");
    if (tcp_connections != NULL)
    {
        debug("TCP init already called once.\n");
        return;
        //TODO: Close and deallocate connection.
    }
    //Listening connection
    listening_connection = NULL;
    
    //No open connections.
    n_tcp_connections = 0;
    tcp_connections = NULL;
    
    //Set max connections
    debug("Setting max. TCP connections: %d.\n", TCP_MAX_CONNECTIONS);
    ret = espconn_tcp_set_max_con(TCP_MAX_CONNECTIONS);
#ifdef DEBUG
    print_status(ret);
#endif
}

