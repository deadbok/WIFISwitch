/** @file http-tcp.c
 *
 * @brief TCP functions for the HTTP server.
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
#include "c_types.h"
#include "osapi.h"
#include "user_config.h"
#include "net/tcp.h"
#include "http-common.h"
#include "http-request.h"
#include "http-response.h"
#include "http.h"
#include "http-tcp.h"

/**
 * @brief Ring buffer for incomming request, that are waiting for a response.
 */
struct ring_buffer request_buffer;

/**
 * @brief Response handler mutex, add one when handling request, substract one when done.
 */
int http_response_mutex = 0;

/**
 * @brief Callback when a connection is made.
 * 
 * Called whenever a connections is made. Sets up the data structures for
 * the server.
 * 
* @param connection Pointer to the connection that has connected.
 */
void ICACHE_FLASH_ATTR tcp_connect_cb(struct tcp_connection *connection)
{
    struct http_request *request;
    
    debug("HTTP new connection (%p).\n", connection);
   
    //Allocate memory for the request data, and tie it to the connection.
    request = (struct http_request *)db_zalloc(sizeof(struct http_request), "request tcp_connect_cb"); 
    debug(" Allocated memory for request data: %p.\n", request);
    //Reset send buffer position.
    request->response.send_buffer_pos = request->response.send_buffer;
    connection->user = request;
    request->connection = connection;
    request->response.status_code = 200;
}

/**
 * @brief Called on connection error.
 * 
 * @param connection Pointer to the connection that has had an error.
 */
void ICACHE_FLASH_ATTR tcp_reconnect_cb(struct tcp_connection *connection)
{  
    debug("HTTP reconnect (%p).\n", connection);
    
    http_process_response(connection);
}

/**
 * @brief Callback when a connection is broken.
 * 
 * Called whenever someone disconnects. Frees up the HTTP data, used by the 
 * connection.
 * 
 * @param connection Pointer to the connection that has disconnected. 
 */
void ICACHE_FLASH_ATTR tcp_disconnect_cb(struct tcp_connection *connection)
{
	struct http_request *request;
	
    debug("HTTP disconnect (%p).\n", connection);
    
    if (connection)
    {
		if (!connection->closing)
		{
			connection->closing = true;
			
			if (connection->user)
			{
				request = connection->user;
				//Call clean up if not yet done.
				if (request->response.state != HTTP_STATE_DONE)
				{
					request->response.state = HTTP_STATE_DONE;
					//Call one last time to clean up.
					http_process_response(connection);
				}
				debug("Clean up done.\n");
			}
			else
			{
				debug("Empty request.\n");
			}
		}
		else
		{
			debug(" Connection is already closing.\n");
		}
	}
	else
	{
		warn("Empty connection.\n");
	}
}

/**
 * @brief Called when a write is done I guess?
 * 
 * @param connection Pointer to the connection that is finished.
 */
void ICACHE_FLASH_ATTR tcp_write_finish_cb(struct tcp_connection *connection)
{
	debug("Done writing (%p).\n", connection);
}

/**
 * @brief Called when data has been received.
 * 
 * Parse the HTTP request, and send a meaningful answer.
 * 
 * @param connection Pointer to the connection that received the data.
 */
void ICACHE_FLASH_ATTR tcp_recv_cb(struct tcp_connection *connection)
{
	void *buffer_ptr;
	
	struct http_request *request = connection->user;
	
    debug("HTTP received (%p).\n", connection);
    if ((connection->callback_data.data == NULL) ||
		(os_strlen(connection->callback_data.data) == 0) ||
		(connection->callback_data.length == 0))
    {
		request->response.status_code = 400;
	}
    if (request->response.status_code == 400)
    {
		warn("Empty request received.<n");
	}

	//A bad idea to parse later since the data apparently may be gone.
	if (!http_parse_request(connection, connection->callback_data.length))
	{
		warn("Parsing failed.\n");
		if (request->response.status_code < 399)
		{
			//Set internal error status.
			request->response.status_code = 404;
			//Use internal error handler.
			request->response.state = HTTP_STATE_ERROR;
		}
	}
	
    debug(" Response handler mutex %d.\n", http_response_mutex);
    if ((!http_response_mutex) && (request_buffer.count < 1))
    {
  
		http_response_mutex++;
		//No request waiting just process.
		http_process_response(connection);
	}
	else
	{
		if (request_buffer.count < (HTTP_REQUEST_BUFFER_SIZE - 1))
		{
			debug(" Adding request to buffer.\n");
			buffer_ptr = ring_push_back(&request_buffer);
			*((struct tcp_connection **)buffer_ptr) = connection;
		}
		else
		{
			error("Dumping request, no free buffers.\n");
		}
	}
    debug(" Request %p done.\n", request);
}

void ICACHE_FLASH_ATTR tcp_sent_cb(struct tcp_connection *connection )
{
	struct http_request *request = connection->user;
	//void *buffer_ptr;
		
	debug("HTTP send (%p).\n", connection);
	
	request->response.send_buffer_pos = request->response.send_buffer;
	//This was the end of a message. Signal that it is send.
	debug(" Response state: %d.\n", request->response.state);
	if (request->response.state == HTTP_STATE_ASSEMBLED)
	{
		http_process_response(connection);
		debug(" Message end.\n");
		request->response.state = HTTP_STATE_DONE;
		//Call one last time to clean up.
		http_process_response(connection);
	}
	else
	{
		//Send some more, we're not done yet.
		http_process_response(connection);
	}
}
