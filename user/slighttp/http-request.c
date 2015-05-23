/** @file http-reqeust.c
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
#include "fs/fs.h"
#include "http-common.h"
#include "http-response.h"
#include "http.h"

/**
 * @brief Handle a GET request.
 * 
 * @param connection Pointer to the connection used.
 * @param headers_only Send only headers, make this usable for HEAD requests
 *                     as well.
 */
void ICACHE_FLASH_ATTR handle_GET(struct tcp_connection *connection, bool headers_only)
{
    struct http_request *request = connection->free;
    unsigned short  i;
    FS_FILE_H file;
    long data_size;
    char *buffer;
    
    file = fs_open(request->uri);
    if (file > FS_EOF)
    {
        debug("Found file: %s.\n", request->uri);
        data_size = fs_size(file);
        buffer = db_malloc((int)data_size + 1);
        fs_read(buffer, data_size, sizeof(char), file);
        buffer[data_size] = '\0';
        send_response(connection, HTTP_RESPONSE(200), 
                      buffer,
                      !headers_only, HTTP_CLOSE_CONNECTIONS);
        db_free(buffer);
        return;
    }
    
    //Check in static uris, go through and stop if URIs match.
    for (i = 0; 
         ((i < n_static_uris) && (!static_uris[i].test_uri(request->uri))); 
         i++);
    
    //Send response if URI is found
    if (i < n_static_uris)
    {
        send_response(connection, HTTP_RESPONSE(200), 
                      static_uris[i].handler(request->uri, request),
                      !headers_only, HTTP_CLOSE_CONNECTIONS);
        return;
    }
    //Send 404
    send_response(connection, HTTP_RESPONSE(404), HTTP_RESPONSE_HTML(404),
                  !headers_only, HTTP_CLOSE_CONNECTIONS);
}

/**
 * @brief Parse headers of a request.
 * 
 * @param connection Pointer to the connection used.
 * @param raw_headers Pointer to the raw headers to parse. *This memory is
 *                    modified*.
 * @return Pointer to the first character after the header and the CRLFCRLF
 */
char ICACHE_FLASH_ATTR *parse_header(struct tcp_connection *connection,
                                           char* raw_headers)
{
    //Pointers to keep track of where we are.
    char *data = raw_headers;
    char *next_data = raw_headers;
    char *value;
    //Where a single header pointers are stored.
    struct http_header *header = NULL;
    //Array where all headers are pointed to.
    struct http_header **headers;
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
    headers = db_zalloc(sizeof(struct http_header *) * n_headers);
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
                send_response(connection, HTTP_RESPONSE(400),
                              HTTP_RESPONSE_HTML(400), true,
                              HTTP_CLOSE_CONNECTIONS);
                return(NULL);
            }
            //Spec. says to return 400 on space before the colon.
            if (*(value - 1) == ' ')
            {
                send_response(connection, HTTP_RESPONSE(400),
                              HTTP_RESPONSE_HTML(400), true,
                              HTTP_CLOSE_CONNECTIONS);
                return(NULL);
            }
            //End name string.
            *value++ = '\0';
            //Convert to lower case.
            data = strlwr(data);
            //Get some memory for the pointers.
            header = (struct http_header *)db_zalloc(sizeof(struct http_header));
            header->name = data;
            debug(" Name (%p): %s\n", header->name, header->name);
            
            //Eat spaces in front of value.
            HTTP_EAT_SPACES(value);
            //Save value.
            header->value = value;
            debug(" Value (%p): %s\n", header->value, header->value);
            //Insert the header.
            headers[n_headers++] = header;
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
char ICACHE_FLASH_ATTR *parse_HEAD(struct tcp_connection *connection, 
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
        send_response(connection, HTTP_RESPONSE(400), HTTP_RESPONSE_HTML(400),
                      true, HTTP_CLOSE_CONNECTIONS);
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
        send_response(connection, HTTP_RESPONSE(400), HTTP_RESPONSE_HTML(400),
                      true, HTTP_CLOSE_CONNECTIONS);
        return(NULL);
    }
    HTTP_EAT_CRLF(next_entry, 1);
    //Save version.  
    request->version = request_entry;
    debug(" Version (%p): %s\n", request_entry, request->version);
    
    return(parse_header(connection, next_entry));
}

