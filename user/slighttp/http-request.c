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
#include "slighttp/http-common.h"
#include "slighttp/http-response.h"
#include "slighttp/http-handler.h"
#include "slighttp/http.h"
#include "slighttp/http-request.h"

/**
 * @brief Get request type (method) and put it in the the request structure.
 * 
 * @param connection Pointer to the connection used.
 * @return Size of the method string.
 */
static size_t http_get_request_type(struct net_connection *connection)
{
    struct http_request *request = connection->user;
    /* Cast first 4 bytes of string to and uint and use that in a switch
     * statement.
     */
    unsigned int *method = (unsigned int *)connection->callback_data.data;

	debug("Request method 0x%x.\n", *method);
	switch(*method)
	{
		case 0x20544547:
			debug("GET request.\n");
			request->type = HTTP_GET;
			return(4);
		case 0x20545550:
			debug("PUT request.\n");
			request->type = HTTP_PUT;
			return(4);
		case 0x34534f50:
			debug("POST request.\n");
			request->type = HTTP_POST;
			return(5);
		case 0x44414548:
			debug("HEAD request.\n");
			request->type = HTTP_HEAD;
			return(5);
		case 0x43415254:
			debug("TRACE request.\n");
			request->type = HTTP_TRACE;
			return(5);
		case 0x454c4544:
			debug("DELETE request.\n");
			request->type = HTTP_DELETE;
			return(6);
		case 0x4e4e4f43:
			debug("CONNECT request.\n");
			request->type = HTTP_CONNECT;
			return(7);
		default:
			error("Unknown request: %s\n", connection->callback_data.data);
			request->response.status_code = 400;
	}
	return(0);
}

/**
 * @brief Parse the headers into the request struct.
 * 
 * *Modifies request data.*
 * 
 * @param request Pointer to the request where the headers belong.
 * @param raw_headers Pointer to the raw headers to parse. 
 */
static void http_parse_headers(
	struct http_request *request,
	char* raw_headers
)
{
	struct http_header *header, *headers = request->headers;
    //Pointers to keep track of where we are.
    char *data = raw_headers;
    char *next_data, *value;
    //True if something is done doing stuff.
    bool done;
    //True if a host header was seen.
    bool host = false;
    
	//Run through the response headers.
	debug("Getting headers (%p).\n", data);
	//Not done.
	done = false;
	//Go go go.
	while (!done && data)
	{
		next_data = strchrs(data, "\r\n");
		if (next_data)
		{
			//Is it the end?
			if ((os_strncmp(next_data, "\r\n\r\n ", 4) == 0) ||
				(os_strncmp(next_data, "\n\n ", 2) == 0))
			{
				debug(" Last header.\n");
				next_data = http_eat_crlf(next_data, 2);
				//Get out.
				done = true;
			}
			else
			{
				next_data = http_eat_crlf(next_data, 1);
			}
			debug("%s\n", data);
			//Find end of name.
            value = os_strstr(data, ":");
            //Spec. says to return 400 on space before the colon.
            if ((!value) || (*(value - 1) == ' '))
            {
                error("Could not parse request header: %s\n", data);
				request->response.status_code = 400;
				return;
			}
			//End name string.
            *value++ = '\0';
            //Convert to lower case.
            //Note to self, the name has just been zero terminated above.
            data = strlwr(data);
            if (os_strcmp(data, "host") == 0)
            {
				debug(" Host header.\n");
				host = true;
			}
            //Get some memory for the pointers.
            header = (struct http_header *)db_zalloc(sizeof(struct http_header), "http_get_headers header");
            //And the name.
            header->name = db_zalloc(os_strlen(data) + 1, "http_get_headers header->name");
            os_strcpy(header->name, data);
            debug(" Name (%p): %s\n", header->name, header->name);
            
            //Eat spaces in front of value.
            HTTP_EAT_SPACES(value);
            //And the value.
            header->value = db_zalloc(os_strlen(value) + 1, "http_get_headers header->value");
            os_strcpy(header->value, value);
            debug(" Value (%p): %s\n", header->value, header->value);
            
			//Add header to list.
			debug(" Adding header");
			if (!request->headers)
			{
				debug(", list starts at %p.\n", header);
				request->headers = header;
				headers = header;
			}
			else
			{
				debug(" at %p.\n", headers);
				DL_LIST_ADD_END(header, headers);
			}
		}
		else
		{
			error("Unexpected or missing end of request headers.\n");
			request->response.status_code = 400;
			return;
		}
		data = next_data;
	}
	if (!host)
	{
		error("No host header.\n");
		request->response.status_code = 400;
		return;
	}
}

/**
 * @brief Parse the request-line, header fields, and save message.
 * 
 * Parse the request-line and header fields of a HTTP request. Put the whole thing
 * in a #http_request and add it to the #tcp_connection data. Any additional
 * data ends up in request.message.
 * 
 * @param connection Pointer to the connection data.
 * @return `true`on success.
 */
bool http_parse_request(struct net_connection *connection, unsigned short length)
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
    
    next_entry = http_skip_crlf(next_entry, 1);
    http_parse_headers(request, next_entry);
	
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
 * @brief Free data allocated for request headers.
 * 
 * @param request Pointer to the request to free headers in.
 */
void http_free_request_headers(struct http_request *request)
{
	if (request->headers)
	{
		debug("Deallocating request headers.\n");
		
		do
		{
			struct http_header *next = request->headers->next;
			
			db_free(request->headers->name);
			db_free(request->headers->value);
			db_free(request->headers);
			request->headers = next;
		} while (request->headers);
	}
}
/**
 * @brief Free data allocated by a request.
 * 
 * @param request Pointer to the request to free.
 */
void http_free_request(struct http_request *request)
{
	debug("Freeing request data at %p.\n", request);
	if (request)
	{
		if (request->response.message)
		{
			debug("Deallocating response message.\n");
			db_free(request->response.message);
		}
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
		http_free_request_headers(request);
		debug("Deallocating request.\n");
		db_free(request);
	}
}
