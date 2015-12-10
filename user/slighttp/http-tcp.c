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
#include "net/net.h"
#include "net/tcp.h"
#include "slighttp/http-common.h"
#include "slighttp/http-handler.h"
#include "slighttp/http-request.h"
#include "slighttp/http-response.h"
#include "slighttp/http.h"
#include "slighttp/http-tcp.h"

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
 * the server. Status code is set to 200. Be positive, nothing will go
 * wrong, I tell you.
 * 
 * @param connection Pointer to the connection that has connected.
 */
void tcp_connect_cb(struct tcp_connection *connection)
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
void tcp_reconnect_cb(struct tcp_connection *connection)
{  
    debug("HTTP reconnect (%p).\n", connection);
}

/**
 * @brief Callback when a connection is broken.
 * 
 * Called whenever someone disconnects. Frees up the HTTP data, used by the 
 * connection.
 * 
 * @param connection Pointer to the connection that has disconnected. 
 */
void tcp_disconnect_cb(struct tcp_connection *connection)
{
    debug("HTTP disconnect (%p).\n", connection);
}

/**
 * @brief Called when a write is done I guess?
 * 
 * @param connection Pointer to the connection that is finished.
 */
void tcp_write_finish_cb(struct tcp_connection *connection)
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
void tcp_recv_cb(struct tcp_connection *connection)
{
	void *buffer_ptr;
	signed int ret;
	
	struct http_request *request = connection->user;
	
    debug("HTTP received (%p).\n", connection);
    if ((connection->callback_data.data == NULL) ||
		(os_strlen(connection->callback_data.data) == 0) ||
		(connection->callback_data.length == 0))
    {
		request->response.status_code = 400;
		warn("Empty request received.<n");
	}

	//A bad idea to parse later since the data may be gone.
	if (!http_parse_request(connection, connection->callback_data.length))
	{
		warn("Parsing failed.\n");
		if (request->response.status_code < 399)
		{
			//Set internal error status.
			request->response.status_code = 400;
		}
	}
	
	//Get the first handler.
	request->response.handler = http_get_handler(request, NULL);

	//Put in buffer if there is stuff there already.
	if ((request_buffer.count > 0) || net_sending)
	{
		if (request_buffer.count < (HTTP_REQUEST_BUFFER_SIZE - 1))
		{
			debug(" Adding request to buffer.\n");
			buffer_ptr = ring_push_back(&request_buffer);
			*((struct tcp_connection **)buffer_ptr) = connection;
			return;
		}
		else
		{
			error("Dumping request, no free buffers.\n");
			return;
		}
	}
	else
	{
		//Start response.
		ret = http_handle_response(request);
		debug(" Handler return value: %d.\n", ret);
	}
    debug(" Request %p done.\n", request);
}

void tcp_sent_cb(struct tcp_connection *connection)
{
	struct http_request *request = connection->user;
	signed int ret = 0;
	void *connection_ptr;
		
	debug("HTTP send (%p).\n", connection);
	//Reset send buffer.
	request->response.send_buffer_pos = request->response.send_buffer;
	debug(" Response state: %d.\n", request->response.state);
	
	//Call handler again.
	if (request->response.handler == NULL)
	{
		/* This should never happen, since the only way to get here, is
		 * when a handler has sent something.
		 */
		error(" No handler.\n");
		return;
	}
	ret = http_handle_response(request);
	debug(" Handler return value: %d.\n", ret);
	
	//Go on if no data was send.
	if (ret <= 0)
	{
		//Answer buffered request.
		debug(" %d buffered Requests.\n", request_buffer.count); 
		if (request_buffer.count > 0)
		{
			debug(" Handling request from buffer.\n");
			connection_ptr = ring_pop_front(&request_buffer);
			if (connection_ptr)
			{
				request = (*((struct tcp_connection **)connection_ptr))->user;
				ret = http_handle_response(request);
				debug(" Handler return value: %d.\n", ret);
				db_free(connection_ptr);
			}
		}
	}
}
