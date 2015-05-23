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
    request = (struct http_request *)db_zalloc(sizeof(struct http_request)); 
    debug(" Allocated memory for request data: %p.\n", request);
    connection->free = request;
}

/**
 * @brief Called on connection error.
 * 
 * @param connection Pointer to the connection that has had an error.
 */
void ICACHE_FLASH_ATTR tcp_reconnect_cb(struct tcp_connection *connection)
{
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
    unsigned int i;
    struct http_request *request = connection->free;
    
    debug("HTTP disconnect (%p).\n", connection);
    
    debug(" Freeing headers.\n");    
    for (i = 0; i < request->n_headers; i++)
    {
        db_free(request->headers[i]);
    }
    debug(" Freeing header array.\n");
    db_free(request->headers);
    debug(" Freeing request data.\n");
    db_free(connection->free);
    connection->free = NULL;
}

/**
 * @brief Called when a write is done I guess?
 * 
 * @param connection Pointer to the connection that is finished.
 */
void ICACHE_FLASH_ATTR tcp_write_finish_cb(struct tcp_connection *connection)
{
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
    struct http_request *request = connection->free;
    
    debug("HTTP received (%p).\n", connection);
    if ((connection->callback_data.data == NULL) || 
        (os_strlen(connection->callback_data.data) == 0))
    {
        error("Emtpy request recieved.\n");
        send_response(connection, HTTP_RESPONSE(400), HTTP_RESPONSE_HTML(400),
                      true, HTTP_CLOSE_CONNECTIONS);
        return;
    }
    //Parse the request header
	if (os_strncmp(connection->callback_data.data, "GET ", 4) == 0)
    {
        debug("GET request.\n");
        request->type = HTTP_GET;
        if (parse_HEAD(connection, 4))
        {
            handle_GET(connection, false);
        }
	}
    else if(os_strncmp(connection->callback_data.data, "POST ", 5) == 0)
    {
        debug("POST request.\n");
        request->type = HTTP_POST;
        request->message = parse_HEAD(connection, 5);
        if (request->message)
        {
            handle_GET(connection, false);
        }
	}
    else if(os_strncmp(connection->callback_data.data, "HEAD ", 5) == 0)
    {
        debug("HEAD request.\n");
        request->type = HTTP_HEAD;
        if (parse_HEAD(connection, 5))
        {
            handle_GET(connection, true);
        }
	}
    else if(os_strncmp(connection->callback_data.data, "PUT ", 4) == 0)
    {
        debug("PUT request.\n");
        request->type = HTTP_PUT;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501),
                      true, HTTP_CLOSE_CONNECTIONS);      
	}
    else if(os_strncmp(connection->callback_data.data, "DELETE ", 7) == 0)
    {
        debug("DELETE request.\n");
        request->type = HTTP_DELETE;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501),
                      true, HTTP_CLOSE_CONNECTIONS);
	}
    else if(os_strncmp(connection->callback_data.data, "TRACE ", 6) == 0)
    {
        debug("TRACE request.\n");
        request->type = HTTP_TRACE;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501),
                      true, HTTP_CLOSE_CONNECTIONS);
	}
    else if(os_strncmp(connection->callback_data.data, "CONECT ", 5) == 0)
    {
        debug("CONNECT request.\n");
        request->type = HTTP_CONNECT;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501),
                      true, HTTP_CLOSE_CONNECTIONS);
	}
    else
    {
        error("Unknown request: %s\n", connection->callback_data.data);
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501),
                      true, HTTP_CLOSE_CONNECTIONS);
    }
}

void ICACHE_FLASH_ATTR tcp_sent_cb(struct tcp_connection *connection )
{
}
