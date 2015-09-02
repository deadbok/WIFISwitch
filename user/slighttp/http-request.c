            /** @file http-request.c
 *
 * @brief Dealing with requests for the HTTP server.
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
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "net/tcp.h"
#include "tools/strxtra.h"
#include "http-common.h"
#include "http-response.h"
#include "http-handler.h"
#include "http.h"
#include "http-request.h"

/**
 * @brief Get request type (method) and put it in the the request structure.
 * 
 * @param connection Pointer to the connection used.
 * @return Size of the method string.
 */
static size_t ICACHE_FLASH_ATTR http_get_request_type(struct tcp_connection *connection)
{
    struct http_request *request = connection->user;
    size_t offset = 0;

    offset = 4;
	if (os_strncmp(connection->callback_data.data, "GET ", offset) == 0)
    {
        debug("GET request.\n");
        request->type = HTTP_GET;
	}
	else if(os_strncmp(connection->callback_data.data, "PUT ", offset) == 0)
    {
        debug("PUT request.\n");
        request->type = HTTP_PUT;
    }
    else
    {
		offset++;
		if(os_strncmp(connection->callback_data.data, "POST ", offset) == 0)
		{
			debug("POST request.\n");
			request->type = HTTP_POST;
		}
		else if(os_strncmp(connection->callback_data.data, "HEAD ", offset) == 0)
		{
			debug("HEAD request.\n");
			request->type = HTTP_HEAD;
		}
		else
		{
			offset++;
			if(os_strncmp(connection->callback_data.data, "TRACE ", offset) == 0)
			{
				debug("TRACE request.\n");
				request->type = HTTP_TRACE;
			}
			else
			{
				offset++;			
				if(os_strncmp(connection->callback_data.data, "DELETE ", offset) == 0)
				{
					debug("DELETE request.\n");
					request->type = HTTP_DELETE;
				}
				else
				{
					offset++;
					if(os_strncmp(connection->callback_data.data, "CONNECT ", offset) == 0)
					{
						debug("CONNECT request.\n");
						request->type = HTTP_CONNECT;
					}
					else
					{
						error("Unknown request: %s\n", connection->callback_data.data);
						request->response.status_code = 501;
						return(0);
					}
				}
			}
		}
	}
	return(offset);
}

/**
 * @brief Parse headers of a request.
 * 
 * @param request Pointer to the request where the headers belong.
 * @param raw_headers Pointer to the raw headers to parse. *This memory is
 *                    modified*.
 * @return Pointer to the first character after the header and the CRLFCRLF
 *         or
 */
static char ICACHE_FLASH_ATTR *http_parse_headers(struct http_request *request,
												  char* raw_headers)
{
    //Pointers to keep track of where we are.
    char *data = raw_headers;
    char *next_data = raw_headers;
    //True if something is done doing stuff.
    bool done;
    
	//Run through the response headers.
	debug("Parsing headers (%p):\n", data);
	debug(" Header callback %p.\n", request->response.handlers->header_cb);
	//Not done.
	done = false;
	//Go go go.
	while (!done && next_data)
	{
		next_data = strchrs(next_data, "\r\n");
		if (next_data)
		{
			//Is it the end?
			if ((os_strncmp(next_data, "\r\n\r\n ", 4) == 0) ||
				(os_strncmp(next_data, "\n\n ", 2) == 0))
			{
				debug(" Last header.\n");
				HTTP_SKIP_CRLF(next_data, 2);
				//Get out.
				done = true;
			}
			else
			{
				//Do bad things to the actual SDK buffer.
				//Replace CRLF with \0 to separate the headers.
				HTTP_EAT_CRLF(next_data, 1);
			}
			if (request->response.handlers->header_cb)
			{
				request->response.handlers->header_cb(request, data);
			}
			else
			{
				warn("No header callback.\n");
				return(NULL);
			}
			//Go to the next entry
			data = next_data;
		}
		else
		{
			debug("Done (%p).", next_data);
		}
	}
	return(next_data);
}

/**
 * @brief Parse the request-line and header fields.
 * 
 * Parse the request-line and header fields of a HTTP request. Put the whole thing
 * in a #http_request and add it to the #tcp_connection data. Any additional
 * data ends up in request.message.
 * 
 * @param connection Pointer to the connection data.
 * @return `true`on success.
 */
bool ICACHE_FLASH_ATTR http_parse_request(struct tcp_connection *connection, unsigned short length)
{
    struct http_request *request = connection->user;
    char *request_entry, *next_entry;
	size_t size;
	
	debug("Parsing request line (%p):\n", connection->callback_data.data);
	size = http_get_request_type(connection);
	if (!size)
	{
		return(false);
	}
    //Start after method.
    request_entry = connection->callback_data.data + size;
    //Parse the rest of request line.
    //Eat spaces to be tolerant, like spec says.
    HTTP_SKIP_SPACES(request_entry);
    //Find the space after the URI, and end the URI string.
    next_entry = os_strstr(request_entry, " ");
    if (next_entry == NULL)
    {
        error("Could not parse HTTP request URI (%s).\n", request_entry);
        return(false);
    }
    
    //Get mem and save uri.
    size = next_entry - request_entry;
    request->uri = db_malloc(size + 1, "request->uri http_parse_request");
    os_strncpy(request->uri, request_entry, size);
    request->uri[size] = '\0';
    debug(" URI (%p): %s\n", request_entry, request->uri); 
     
    HTTP_SKIP_SPACES(next_entry);
    request_entry = next_entry;
    //Check 'HTTP/' and save version.
    if (os_memcmp(request_entry, "HTTP/", 5) != 0)
    {
        error("Could not parse HTTP request version (%s).\n", request_entry);
        return(false);
    }
    
    request_entry += 5;
    //Find the CR after the version, and end the string.
    next_entry = strchrs(next_entry, "\r\n");
    if (next_entry == NULL)
    {
        error("Could not parse HTTP request version (%s).\n", request_entry);
        return(false);
    }
    
    //Get mem and save version.
    size = next_entry - request_entry;
    request->version = db_malloc(size + 1, "request->version http_parse_request");
    os_strncpy(request->version, request_entry, size);
    request->version[size] = '\0';
    debug(" Version (%p): %s\n", request_entry, request->version);
    
    HTTP_SKIP_CRLF(next_entry, 1);
    
    request->response.handlers = http_get_handler(request);
    if (!request->response.handlers)
    {
		warn("No request handlers.\n");
		return(false);
	}
    if (!request->response.handlers->request_cb)
    {
		warn("Could not find request handler.\n");
		return(false);
	}
	debug(" Header callback %p.\n", request->response.handlers->request_cb);
	request->response.handlers->request_cb(request);
	
    next_entry = http_parse_headers(request, next_entry);
    if (!next_entry)
    {
		warn("Could not parse headers.\n");
		return(false);
	}
        
    //Get length of message data if any.
    size = length - (next_entry - connection->callback_data.data);
    debug(" Message length: %d.\n", size);
    if (size)
    {

		request->message = db_malloc(sizeof(char) * (size + 1), "request->message http_parse_request");
		memcpy(request->message, next_entry, size);
		request->message[size] = '\0';
		debug("%s\n", request->message);
	}

    debug(" Done parsing request.\n");
    return(true);
}

/**
 * @brief Free data allocated by a request.
 * 
 * @param request Pointer to the request to free.
 */
void ICACHE_FLASH_ATTR http_free_request(struct http_request *request)
{
	debug("Freeing request data at %p.\n", request);
	if (request)
	{
		if (request->uri)
		{
			debug("Deallocating request URI %s.\n", request->uri);
			db_free(request->uri);
		}
		if (request->version)
		{
			debug("Deallocating request version %s.\n", request->version);
			db_free(request->version);
		}
		if (request->message)
		{
			debug("Deallocating request message.\n");
			db_free(request->message);
		}
		debug("Deallocating request.\n");
		db_free(request);
	}
}
