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
#include "http.h"
#include "http-request.h"

/**
 * @brief Parse headers of a request.
 * 
 * @param connection Pointer to the connection used.
 * @param raw_headers Pointer to the raw headers to parse. *This memory is
 *                    modified*.
 * @return Pointer to the first character after the header and the CRLFCRLF
 */
char ICACHE_FLASH_ATTR *http_parse_headers(struct tcp_connection *connection,
                                           char* raw_headers)
{
    //Pointers to keep track of where we are.
    char *data = raw_headers;
    char *next_data = raw_headers;
    char *value;
    //Array where all headers are pointed to.
    struct http_header *headers;
    //Number of headers.
    unsigned short n_headers;
    //True if something is done doing stuff.
    bool done;
    
    //Count the headers
    debug("Counting headers (%p)", raw_headers);
    //We're NOT done!
    done = false;
    //Told you, see, no headers.
    n_headers = 0;
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
                n_headers++;
                debug(": %d\n", n_headers);
                //Get out.
                done = true;
            }
            else
            {
                //...or just another header.
                n_headers++;
                debug(".");
            }
            next_data++;
            if (*next_data == '\n')
            {
                next_data++;
            }
        }
    }
    //Go back to where the headers start.
    next_data = raw_headers;
    //Run through the response headers.
    debug("Parsing headers (%p):\n", data);
    //Allocate memory for the array of headers
    headers = db_malloc(sizeof(struct http_header) * n_headers, "headers http_parse_headers");
    //Start from scratch.
    n_headers = 0;
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
                HTTP_EAT_CRLF(next_data, 2);
                //Get out.
                done = true;
            }
            else
            {
                HTTP_EAT_CRLF(next_data, 1);
            }
            //Find end of name.
            value = os_strstr(data, ":");
            if (!value)
            {
                error("Could not parse request header: %s\n", data);
                //Bail out
                return(NULL);
            }
            //Spec. says to return 400 on space before the colon.
            if (*(value - 1) == ' ')
            {
                return(NULL);
            }
            //End name string.
            *value++ = '\0';
            //Convert to lower case.
            data = strlwr(data);
            //Get some memory for the pointers.
            headers[n_headers].name = data;
            debug(" Name (%p): %s\n", headers[n_headers].name, 
				  headers[n_headers].name);
            
            //Eat spaces in front of value.
            HTTP_EAT_SPACES(value);
            //Save value.
            headers[n_headers].value = value;
            debug(" Value (%p): %s\n", headers[n_headers].value,
				  headers[n_headers].value);
            //Go to the next entry
            data = next_data;
        }
        else
        {
            debug("Done (%p).", next_data);
        }
    }
    ((struct http_request *)connection->free)->headers = headers;
    ((struct http_request *)connection->free)->n_headers = n_headers;
    return(next_data);
}

/**
 * @brief Parse the start-line and header fields.
 * 
 * Parse the start-line and header fields of a HTTP request. Put the whole thing
 * in a #http_request and add it to the #tcp_connection data.
 * 
 * @param connection Pointer to the connection data.
 * @param start_offset Where to start parsing the data.
 * @return Pointer to the start of the raw message.
 */
char ICACHE_FLASH_ATTR *http_parse_request(struct tcp_connection *connection, 
                                         unsigned char start_offset)
{
    struct http_request *request = connection->free;
    char *request_entry, *next_entry;

    //Start after method.
    request_entry = connection->callback_data.data + start_offset;
    //Parse the rest of request line.
    debug("Parsing request line (%p):\n", request_entry);

    //Eat spaces to be tolerant, like spec says.
    HTTP_EAT_SPACES(request_entry);
    //Find the space after the URI, and end the URI string.
    next_entry = os_strstr(request_entry, " ");
    if (next_entry == NULL)
    {
        error("Could not parse HTTP request URI (%s).\n", request_entry);
        return(NULL);
    }
    HTTP_EAT_SPACES(next_entry);
    //Save URI
    request->uri = request_entry;
    debug(" URI (%p): %s\n", request_entry, request->uri);  
    
    request_entry = next_entry;
    //Skip 'HTTP/' and save version.
    request_entry += 5;
    //Find the CR after the version, and end the string.
    next_entry = strchrs(next_entry, "\r\n");
    if (next_entry == NULL)
    {
        error("Could not parse HTTP request version (%s).\n", request_entry);
        return(NULL);
    }
    HTTP_EAT_CRLF(next_entry, 1);
    //Save version.  
    request->version = request_entry;
    debug(" Version (%p): %s\n", request_entry, request->version);
    
    return(http_parse_headers(connection, next_entry));
}

void ICACHE_FLASH_ATTR http_process_request(struct tcp_connection *connection)
{
    struct http_request *request = connection->free;
    struct http_response *response;
    unsigned short status_code = 200;
    bool headers_only = false;
    
    //Parse the request header
	if (os_strncmp(connection->callback_data.data, "GET ", 4) == 0)
    {
        debug("GET request.\n");
        request->type = HTTP_GET;
        if (!http_parse_request(connection, 4))
        {
            status_code = 400;
        }
	}
    else if(os_strncmp(connection->callback_data.data, "POST ", 5) == 0)
    {
        debug("POST request.\n");
        request->type = HTTP_POST;
        request->message = http_parse_request(connection, 5);
        if (!request->message)
        {
            status_code = 400;
        }
	}
    else if(os_strncmp(connection->callback_data.data, "HEAD ", 5) == 0)
    {
        debug("HEAD request.\n");
        request->type = HTTP_HEAD;
        if (!http_parse_request(connection, 5))
        {
            status_code = 400;
        }
        else
        {
			headers_only = true;
		}
	}
    else if(os_strncmp(connection->callback_data.data, "PUT ", 4) == 0)
    {
        debug("PUT request.\n");
        request->type = HTTP_PUT;
        status_code = 501;
    }
    else if(os_strncmp(connection->callback_data.data, "DELETE ", 7) == 0)
    {
        debug("DELETE request.\n");
        request->type = HTTP_DELETE;
        status_code = 501;
	}
    else if(os_strncmp(connection->callback_data.data, "TRACE ", 6) == 0)
    {
        debug("TRACE request.\n");
        request->type = HTTP_TRACE;
		status_code = 501;
   	}
    else if(os_strncmp(connection->callback_data.data, "CONECT ", 5) == 0)
    {
        debug("CONNECT request.\n");
        request->type = HTTP_CONNECT;
        status_code = 501;
	}
    else
    {
        error("Unknown request: %s\n", connection->callback_data.data);
		status_code = 501;
    }
    response = http_generate_response(connection, status_code);
    debug("Response: %p.\n", response);
    http_send_response(response, !headers_only);
    http_free_response(response);
    http_free_request(request);
}

void ICACHE_FLASH_ATTR http_free_request(struct http_request *request)
{
	/*Most pointers can't be freed here, since they point to espconn
	 *owned data.*/
	debug("Freeing request data at %p.\n", request);
    db_free(request->headers);
    db_free(request);
}
