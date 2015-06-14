/**
 * @file tcp.c
 *
 * @brief TCP server routines for the ESP8266.
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
#include "tcp.h"

/**
 * @brief Timer for housekeeping.
 */
static os_timer_t timer;

/**
 * @brief Array to look up a string from connection state.
 */
const char *state_names[] = {"ESPCONN_NONE", "ESPCONN_WAIT", "ESPCONN_LISTEN",\
                             "ESPCONN_CONNECT", "ESPCONN_WRITE", "ESPCONN_READ",\
                             "ESPCONN_CLOSE"};
                             
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
     * @brief Callback on a reconnect.
     * 
     * According to the SDK documentation, this should be considered an error
     * callback. If an error happens somewhere in the espconn code this is 
     * called.
     */
    tcp_callback reconnect_callback;
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
 * @brief Special case connection structure.
 * 
 * Structure used by the listening connection, which includes callback
 * function pointers.
 */
struct tcp_cb_connection
{
    /**
     * @brief Pointer to the `struct espconn` structure associated with the connection.
     */
    struct espconn              *conn;
    /**
     * @brief Pointers to callback functions for handling connection states.
     */
    struct tcp_callback_funcs   callbacks;
    /**
     * @brief Pointer to the data meant for the current callback.
     */
    struct tcp_callback_data    callback_data;
};
	
/**
 * @brief Number of active connections.
 */
static unsigned char n_tcp_connections;
/**
 * @brief Doubly-linked list of active connections.
 */
static struct tcp_connection *tcp_connections = NULL;

/**
 * @brief The listening connection.
 */
static struct tcp_cb_connection *listening_connection;

//Forward declaration of callback functions.
static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_reconnect_cb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR tcp_disconnect_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_write_finish_cb(void *arg);
static void ICACHE_FLASH_ATTR tcp_recv_cb(void *arg, char *data, unsigned short length);
static void ICACHE_FLASH_ATTR tcp_sent_cb(void *arg);
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

/** 
 * @brief Free up data structures used by a connection.
 * 
 * Free up the data pointed to by @p connection. espconn is expected to clean up
 * after itself.
 * @param connection Pointer to the data to free.
 */
void ICACHE_FLASH_ATTR tcp_free(struct tcp_connection *connection)
{
    debug("Freeing up connection (%p).\n", connection);
    /* As far as I can tell the espconn library takes care of freeing up the
     * structures it has allocated. */

    //Remove connection from, the list of active connections.
    DL_LIST_UNLINK(connection, tcp_connections);
    debug(" Unlinked.\n");
    
    //Maybe try to send the data if any (send_buffer != current_buffer_pos)?
    if (connection->send_buffer)
    {
		db_free(connection->send_buffers);
	}
	connection->conn->reverse = NULL;
    if (connection != NULL)
    {
        db_free(connection);
        debug(" Connection data freed.\n");
    }
    n_tcp_connections--;
    debug(" Connections: %d.\n", n_tcp_connections);
}

/**
 * @brief Internal callback for when a new connection has been made.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg)
{
    struct tcp_connection   *connection;
    
    //Check if there are any free connections.
    if (n_tcp_connections < TCP_MAX_CONNECTIONS)
    {
        connection = (struct tcp_connection *)db_zalloc(sizeof(struct tcp_connection), "connection tcp_connect_cb");
        connection->conn = arg;
        
        //Set internal callbacks.
        connection->conn->proto.tcp->connect_callback = tcp_connect_cb;
        connection->conn->proto.tcp->reconnect_callback = tcp_reconnect_cb;
        connection->conn->proto.tcp->disconnect_callback = tcp_disconnect_cb;
        connection->conn->proto.tcp->write_finish_fn = tcp_write_finish_cb;
        connection->conn->recv_callback = tcp_recv_cb;
        connection->conn->sent_callback = tcp_sent_cb;
        
        //No buffer yet.
        connection->send_buffer = NULL;
		connection->current_buffer_pos = NULL;
		connection->sending = 0;
		connection->closing = false;
		connection->buffer_used = 0;
        
        //Save our connection data
        connection->conn->reverse = connection;
        
        debug("TCP connected (%p).\n", connection);

        //Add connection to the list.
        if (tcp_connections == NULL)
        {
            tcp_connections = connection;
            connection->prev = NULL;
            connection->next = NULL;
        }
        else
        {
            struct tcp_connection  *connections = tcp_connections;
            
            DL_LIST_ADD_END(connection, connections);
        }
        //Increase number of connections.        
        n_tcp_connections++;
        //Call callback.
        if (listening_connection->callbacks.connect_callback != NULL)
        {
            listening_connection->callbacks.connect_callback(connection);
        }
    }
    else
    {
        os_printf("No more free TCP connections.\n");
        espconn_disconnect(arg);
    }
    debug(" Connections: %d.\n", n_tcp_connections);
}

/**
 * @brief Internal error handler.
 * 
 * Called whenever an error has occurred.
 * 
 * @param arg Pointer to an espconn connection structure.
 * @param err Error code.
 */
static void ICACHE_FLASH_ATTR tcp_reconnect_cb(void *arg, sint8 err)
{
    struct espconn *conn = arg;
    struct tcp_connection   *connection = conn->reverse;
        
    debug("TCP reconnected (%p).\n", connection);
    os_printf("Error: ");
    print_status(err);
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    connection->callback_data.err = err;
    //Call call back.
    if (listening_connection->callbacks.reconnect_callback != NULL)
    {
        listening_connection->callbacks.reconnect_callback(connection);
    }
}

/**
 * @brief Internal callback for when someone disconnects.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
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
     * up the closing connection.
     */
    struct tcp_connection *connection = tcp_connections;
#ifdef DEBUG
    struct espconn *conn = arg;
#endif //DEBUG
    
    debug("TCP disconnected (%p).\n", conn->reverse);
     
    while (connection != NULL)
    {
		//Update state of connections..
		if (connection->conn->state >= ESPCONN_CLOSE)
		{
#ifdef DEBUG
			if (connection->conn->state > ESPCONN_CLOSE)
			{
				debug("Closing connection %p (%p) that seems to be dangling.\n", connection, connection->conn);
			}
			else
			{
				debug("Closing connection %p (%p) state \"%s\".\n", connection, connection->conn, state_names[connection->conn->state]);
			}
#endif //DEBUG
			connection->closing = true;
		}
        connection = connection->next;
    }
}

/**
 * @brief Internal callback, called when a TCP write is done.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
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
    if (listening_connection->callbacks.write_finish_fn != NULL)
    {
        listening_connection->callbacks.write_finish_fn(connection);
    }    
}

/**
 * @brief Internal callback, called when data is received.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
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
    if (listening_connection->callbacks.recv_callback != NULL)
    {
        listening_connection->callbacks.recv_callback(connection);
    }    
}

/**
 * @brief Switch between first and secondary send buffer.
 * 
 * @param connection Connection to work on.
 */
static void ICACHE_FLASH_ATTR switch_send_buffer(struct tcp_connection *connection)
{
	//Switch to between the first and the second buffer.
	if (connection->send_buffer == connection->send_buffers)
	{
		//Second buffer.
		connection->send_buffer = connection->send_buffers + TCP_SEND_BUFFER_SIZE + 1;
		debug(" Switching to second buffer (%p).\n", connection->send_buffer);
	}
	else
	{
		//First buffer.
		connection->send_buffer = connection->send_buffers;
		debug(" Switching to first buffer (%p).\n", connection->send_buffer);
	}
	connection->current_buffer_pos = connection->send_buffer;
}

/**
 * @brief Send the send buffer.
 * 
 * @param connection Pointer to the connection to handle.
 */
static void ICACHE_FLASH_ATTR send_buffer(struct tcp_connection *connection)
{
	size_t buffer_size = 0;
#ifdef DEBUG
	unsigned short i;
#endif //DEBUG
	
	if (!connection->sending)
	{
		debug("Sending TCP data from buffer (%p).\n", connection);
		
		if (!connection->send_buffer)
		{
			debug(" No buffer (%p).\n", connection->send_buffer);
			connection->sending = false;
			return;
		}
		else if (connection->send_buffer == connection->current_buffer_pos)
		{
			debug(" Buffer is empty (buffer: %p, pos %p).\n",connection->send_buffer, connection->current_buffer_pos);
			connection->sending = false;
			return;
		}
		
		buffer_size = connection->current_buffer_pos - connection->send_buffer;

		connection->sending = buffer_size;
		debug(" Sending buffer at %p, %d bytes.\n", connection->send_buffer, buffer_size);
#ifdef DEBUG
		for (i = 0; i < buffer_size; i++)
		{
			os_putc(connection->send_buffer[i]);
		}
		os_putc('\n');
#endif //DEBUG
		espconn_sent(connection->conn, connection->send_buffer, buffer_size);
		switch_send_buffer(connection);
	}
}

/**
 * @brief Internal callback, called when TCP data has been sent.
 * 
 * @param arg Pointer to an espconn connection structure.
 */
static void ICACHE_FLASH_ATTR tcp_sent_cb(void *arg)
{
    struct espconn *conn = arg;
    struct tcp_connection *connection = conn->reverse;
	char ret;
    
    debug("TCP sent (%p).\n", connection);

	connection->buffer_used -= connection->sending;
	debug(" Bytes of buffer in use: %d\n", connection->buffer_used);
	connection->sending = 0;
	
    //Clear previous data.
    os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    connection->callback_data.arg = arg;
    //Call call back.
    if (listening_connection->callbacks.sent_callback != NULL)
    {
        listening_connection->callbacks.sent_callback(connection);
    }
    //If there is more data in the buffer, send some more.
    if (connection->buffer_used > 0)
    {
		send_buffer(connection);
	}
	//Disconnect if asked, and buffer is empty.
	if ((connection->closing) && (connection->buffer_used == 0))
	{
		debug("Disconnecting (%p)...", connection);
		ret = espconn_disconnect(connection->conn);
#ifdef DEBUG
		print_status(ret);
#endif
	}
}	

/** 
 * @brief Send TCP data.
 * 
 * @note Won't send more than #TCP_SEND_BUFFER_SIZE * 2 - buffer used bytes at a time.
 * 
 * @param connection Connection used for sending the data.
 * @param data Pointer to the data to send.
 * @param size Length of data in bytes.
 * 
 * @return true on success, false otherwise.
 */
bool ICACHE_FLASH_ATTR tcp_send(struct tcp_connection *connection, char *data, size_t size)
{
	size_t buffer_used = 0;
	size_t buffer_free = 0;
	size_t data_left = size;
	
    debug("Queueing %d bytes of outgoing TCP data (%p using %p),\n", size, data, connection);

	if (size > (TCP_SEND_BUFFER_SIZE * 2 - connection->buffer_used))
	{
		warn("Cannot send %d (free send buffer: %d) bytes of TCP data at.\n", size, TCP_SEND_BUFFER_SIZE * 2 - connection->buffer_used);
		return(false);
	}
		
	//Create a buffer if there is none.
    if (!connection->send_buffers)
    {
		connection->send_buffers = db_malloc(TCP_SEND_BUFFER_SIZE * 2, "connection->send_buffers tcp_send");
		debug(" Created new send buffer %p.\n", connection->send_buffer);
		connection->send_buffer = connection->send_buffers;
		connection->current_buffer_pos = connection->send_buffer;
	}
	
	while (data_left > 0)
	{

		buffer_used = connection->current_buffer_pos - connection->send_buffer;
		buffer_free = TCP_SEND_BUFFER_SIZE - buffer_used;
		
		if (data_left > buffer_free)
		{
			debug(" Adding %d bytes to buffer at %p.\n", buffer_free, connection->current_buffer_pos);
			os_memcpy(connection->current_buffer_pos, data, buffer_free);
			connection->current_buffer_pos += buffer_free;
			data_left -= buffer_free;
			connection->buffer_used += buffer_free;
			send_buffer(connection);

		}
		else
		{
			debug(" Adding %d bytes to buffer at %p.\n", data_left, connection->current_buffer_pos);
			os_memcpy(connection->current_buffer_pos, data, data_left);
			connection->current_buffer_pos += data_left;
			connection->buffer_used += data_left;
			data_left = 0;
		}
		debug(" %d bytes left to queue.\n", data_left);
		debug(" %d bytes of buffer in use.\n", connection->buffer_used);		
	}
	return(true);
}

/**
 * @brief Disconnect the TCP connection.
 * 
 * @param connection Connection to disconnect.
 */
void ICACHE_FLASH_ATTR tcp_disconnect(struct tcp_connection *connection)
{
    debug("Disconnect (%p).\n", connection);

	connection->closing = true;
}

/**
 * @brief Create a listening connection.
 * 
 * This creates a listening connection for the TCP server.
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
 */
void ICACHE_FLASH_ATTR tcp_listen(int port, tcp_callback connect_cb, 
                                tcp_callback reconnect_cb, tcp_callback disconnect_cb,
                                tcp_callback write_finish_cb, tcp_callback recv_cb,
                                tcp_callback sent_cb)
{
    int                         ret;
    struct tcp_cb_connection    *connection;
    struct espconn              *conn;
    struct tcp_callback_funcs   *callbacks;
    
    debug("TCP listen.\n");
    //Bail out if somebody is already listening.
    if (listening_connection == NULL)
    {
        //Allocate memory for the new connection.
        connection = (struct tcp_cb_connection *)db_zalloc(sizeof(struct tcp_cb_connection), "connection tcp_listen");
        connection->conn = (struct espconn *)db_zalloc(sizeof(struct espconn), "espconn tcp_listen");
        connection->conn->proto.tcp = (esp_tcp *)db_zalloc(sizeof(esp_tcp), "esp_tcp tcp_listen");
        
        debug(" Created connection %p.\n", connection);        
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
        debug(" Setting connection timeout to 60 secs...");
        //Set timeout for all connections.
        ret = espconn_regist_time(conn, 60, 0);
#ifdef DEBUG
        print_status(ret);
#endif
    }
    else
    {
        error("Only one listening TCP connection supported.\n");
    }
}

/**
 * @brief House keeping timer call back function.
 * 
 * This will close connection, that are not needed, and send the TCP
 * buffer, and send any data in the send buffer.
 */
void tcp_timer_cb(void *unused)
{
    struct tcp_connection *connection = tcp_connections;
     
    while (connection != NULL)
    {
		//Disconnect if asked, and buffer is empty.
		if (((connection->closing) && (connection->buffer_used == 0)) || (connection->conn->state >= ESPCONN_CLOSE))
		{
#ifdef DEBUG
			if (connection->conn->state > ESPCONN_CLOSE)
			{
				debug("Closing connection %p (%p) that seems to be dangling.\n", connection, connection->conn);
			}
			else
			{
				debug("Closing connection %p (%p) state \"%s\".\n", connection, connection->conn, state_names[connection->conn->state]);
			}
#endif //DEBUG
			connection->closing = true;
			//Clear previous data.
            os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
            //Set new data
            connection->callback_data.arg = connection->conn;
            //Call call back.
            if (listening_connection->callbacks.disconnect_callback != NULL)
            {
                listening_connection->callbacks.disconnect_callback(connection);
            }

            //Free the connection
            tcp_free(connection);
		}
		else
		{
#ifdef DEBUG
			if (connection->conn->state > ESPCONN_CLOSE)
			{
				debug("Connection %p (%p) seems to be dangling.\n", connection, connection->conn);
			}
			else
			{
				debug("Connection %p (%p) state \"%s\".\n", connection, connection->conn, state_names[connection->conn->state]);
			}
#endif //DEBUG
			send_buffer(connection);
		}
        connection = connection->next;
    }
}

/**
 * @brief Initialise TCP networking.
 */
void ICACHE_FLASH_ATTR init_tcp(void)
{
    int ret;
    
    debug("TCP init.\n");
    if (tcp_connections != NULL)
    {
        debug("TCP init already called once.\n");
        return;
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

	//Set housekeeping timer.
    //Disarm timer
    os_timer_disarm(&timer);
    //Setup timer.
    os_timer_setfn(&timer, (os_timer_func_t *)tcp_timer_cb, NULL);
    //Arm the timer, run every 1 second.
    os_timer_arm(&timer, 200, true);
}

void ICACHE_FLASH_ATTR tcp_stop(void)
{
	int ret;
	
	debug("Closing TCP connection %p.\n", listening_connection);
	
	ret = espconn_delete(listening_connection->conn);

#ifdef DEBUG
    print_status(ret);
#endif

	os_timer_disarm(&timer);
}
