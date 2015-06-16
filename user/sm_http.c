/**
 * @file sm_http.c
 *
 * @brief WIFI code for the main state machine.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
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
#include "user_interface.h"
#include "net/tcp.h"
#include "slighttp/http.h"
#include "rest/rest.h"
#include "sm_types.h"

/**
 * @brief Number of response handlers.
 */
#define N_RESPONSE_HANDLERS  3

/**
 * @brief Array of built in handlers and their URIs.
 */
static struct http_response_handler response_handlers[N_RESPONSE_HANDLERS] =
{
	{
		http_fs_test,
		{
			NULL,
			http_fs_get_handler,
			http_fs_head_handler,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		}, 
		http_fs_destroy
	},
	{
		rest_network_test,
		{
			NULL,
			rest_network,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		}, 
		rest_network_destroy
	},
    {
		rest_net_names_test,
		{
			NULL,			 
			rest_net_names,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		}, 
		rest_net_names_destroy
	}
};

/**
 * @brief Connect to the configured Access Point, or create an AP, if unsuccessful.
 * 
 * This will keep calling itself through the main state machine until a
 * connection is made, or it times out, and sets up an access point.
 */
state_t ICACHE_FLASH_ATTR http_init(void *arg)
{
	struct sm_context *context = arg;
	
	if (init_http(context->http_root, response_handlers, N_RESPONSE_HANDLERS))
	{
		error("Could not start HTTP server.\n");
		return(SYSTEM_RESTART);
	}
	return(HTTP_SEND);
}

/**
 * @brief Assemble and send waiting responses.
 * 
 * This function is expected to exit, before the send buffer is full, and
 * make room for the stupid event system to actually send the data. Therefore
 * expect to cut up the response, and keep track of how much has been send,
 * using the TCP sent callback.
 */
state_t ICACHE_FLASH_ATTR http_send(void *arg)
{
	static struct tcp_connection *connection = NULL;
    static struct http_request *request;
    
    if (!connection)
    {
		connection = tcp_get_connections();
		if (!connection)
		{
			debug("No connections.\n");
			return(HTTP_CLEANUP);
		}
		debug("No connection, starting from first (%p).\n", connection);
		request = connection->free;
	}
	
	// Since request was zalloced, request->type will be zero until parsed.
	if ((request) && (request->type != HTTP_NONE))
	{
		//Get started
		if (request->response.state == HTTP_STATE_NONE)
		{
			debug("New response.\n");
			//Find someone to respond.
			request->response.state = HTTP_STATE_FIND;
		}
		switch (request->response.state)
		{
			case HTTP_STATE_FIND: request->response.handlers = http_get_handlers(request);
								  //No handler found.
								  if (!request->response.handlers->handlers[request->type - 1])
								  {
									  request->response.status_code = 404;
								  } 
								  //Next state.
								  request->response.state = HTTP_STATE_STATUS;
								  break;
			case HTTP_STATE_STATUS: //Send response
			case HTTP_STATE_HEADERS:
			case HTTP_STATE_MESSAGE: debug("Calling response handler %p.\n",
										   request->response.handlers->handlers[request->type - 1]);
									 request->response.handlers->handlers[request->type - 1](request); 
									 break;
    		case HTTP_STATE_DONE: break;
		}
	}
	else
	{
		debug("No request parsed yet.\n");
	}
     
    connection = connection->next;

	return(HTTP_CLEANUP);
}

/**
 * @brief Do house keeping.
 * 
 * Close and deallocate unused connections.
 * @note This is mostly done here instead of the disconnect callback,
 * because the ESP8266 SDK returns the wrong connection, for what
 * needs to be done.
 */
state_t ICACHE_FLASH_ATTR http_mutter_med_kost_og_spand(void *arg)
{
	static struct tcp_connection *connection = NULL;
    static struct http_request *request;
    
    if (!connection)
    {
		connection = tcp_get_connections();
		if (!connection)
		{
			debug("No connections.\n");
			return(WIFI_CHECK);
		}
		debug("No connection, starting from first (%p).\n", connection);
		request = connection->free;
	}
	
	// Since request was zalloced, request->type will be zero until parsed.
	if ((request) && (request->type != HTTP_NONE))
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
			
			//Old call back, fiure out if it is needed.
			//Clear previous data.
            //os_memset(&connection->callback_data, 0, sizeof(struct tcp_callback_data));
            //Set new data
            //connection->callback_data.arg = connection->conn;
            //Call call back.
            //if (listening_connection->callbacks.disconnect_callback != NULL)
            //{
            //    listening_connection->callbacks.disconnect_callback(connection);
            //}

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
	}
	else
	{
		debug("No request parsed yet.\n");
	}
     
    connection = connection->next;
    
	return(WIFI_CHECK);
}
