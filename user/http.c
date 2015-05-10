/** @file http.c
 *
 * @brief Simple HTTP server for the ESP8266.
 * 
 * Simple HTTP server, that only supports the most basic functionality, and is
 * small.
 * 
 * To keep memory use small, the data passed to the server, from the network, is 
 * expected to stay until the server is done with them. Instead of making a copy
 * of the data from the network, the server inserts `\0` characters directly into
 * the data. The server then point to the values directly in the network data.
 * 
 * What works:
 * - GET requests.
 * - HEAD requests.
 * 
 * **THIS SERVER IS NOT COMPLIANT WITH HTTP, TO SAVE SPACE, STUFF HAS BEEN
 * OMITTED.**
 * 
 * Missing functionality:
 * - Does not understand any header fields.
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
#include <stdlib.h>
#include "missing_dec.h"
#include "osapi.h"
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "tcp.h"
#include "http.h"

/**
 * @brief Number of entries in the HTTP request line, excluding the method.
 */
#define HTTP_REQUEST_ENTRIES     2

/**
 * @brief HTTP request types.
 */
enum request_type
{
    OPTIONS,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    CONNECT
};

/**
 * @brief A HTTP header entry.
 * 
 * To save space, the actual values are not copied here from the received data,
 * rather they are pointed to.
 */
struct http_header
{
    /**
     * @brief Pointer to the name.
     */
    char *name;
    /**
     * @brief Pointer to the value.
     */
    char *value;
};

/**
 * @brief Structure to keep the data of a HTTP request.
 */
struct http_request
{
    /**
     * @brief Pointer to the connection data.
     */
    struct tcp_connection *connection;
    /**
     * @brief Type of HTTP request.
     */
    enum request_type   type;
    /**
     * @brief Where to start parsing the request skipping the type.
     */
    unsigned char       start_offset;
    /**
     * @brief The URI of the HTTP request.
     */
    char                *uri;
    /**
     * @brief The version of the HTTP request.
     */
    char                *version;
    /**
     * @brief Headers of the request.
     */
    struct http_header  **headers;
    /**
     * @brief Number of headers
     */
     unsigned short     n_headers;
    /**
     * @brief The message body of the HTTP request.
     */
    char                *message;
};

static struct http_builtin_uri *static_uris;
static unsigned short n_static_uris;

/**
 * @brief Callback when a connection is made.
 * 
 * Called whenever a connections is made. Sets up the data structures for
 * the server.
 */
static void ICACHE_FLASH_ATTR tcp_connect_cb(struct tcp_connection *connection)
{
    struct http_request *request;
    
    debug("HTTP new connection (%p).\n", connection);
   
    //Allocate memory for the request data, and tie it to the connection.
    request = (struct http_request *)db_zalloc(sizeof(struct http_request)); 
    debug(" Allocated memory for request data: %p.\n", request);
    connection->free = request;
}

static void ICACHE_FLASH_ATTR tcp_reconnect_cb(struct tcp_connection *connection)
{
}

/**
 * @brief Callback when a connection is broken.
 * 
 * Called whenever someone disconnects. Free up the HTTP data, used by the 
 * connection.
 */
static void ICACHE_FLASH_ATTR tcp_disconnect_cb(struct tcp_connection *connection)
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

static void ICACHE_FLASH_ATTR tcp_write_finish_cb(struct tcp_connection *connection)
{
}

/**
 * @brief Send a HTTP response.
 * 
 * @param connection Connection to use when sending.
 * @param response Response header (without the final `\r\n\r\n`.
 * @param content The content of the response.
 * @param send_content If `false` only send headers.
 */
static void ICACHE_FLASH_ATTR send_response(struct tcp_connection *connection,
                                                   char *response,
                                                   char *content,
                                                   bool send_content)
{
    char *h_length = "Content-Length: ";
    char html_length[6];
    
    //Send start of the response.
    tcp_send(connection, response);
    //Send the length header.
    tcp_send(connection, h_length);
    //Get the message length.
    os_sprintf(html_length, "%d", os_strlen(content));
    //Send it.
    tcp_send(connection, html_length);
    //End of header.
    tcp_send(connection, "\r\n\r\n");
    //Send content.
    if (send_content)
    {
        tcp_send(connection, content);
    }
}

/**
 * @biref Handle a GET request.
 */
static void ICACHE_FLASH_ATTR handle_GET(struct tcp_connection *connection, bool headers_only)
{
    struct http_request *request = connection->free;
    unsigned short  i;
    
    //Check in static uris, go through and stop if URIs match.
    for (i = 0; 
         ((i < n_static_uris) && 
         (os_strcmp(request->uri, static_uris[i].uri) != 0)); 
         i++);
    
    //Send response if URI is found
    if (i < n_static_uris)
    {
        send_response(connection, HTTP_RESPONSE(200), 
                      static_uris[i].callback(request->uri), !headers_only);
        return;
    }
    //Send 404
    send_response(connection, HTTP_RESPONSE(404), HTTP_RESPONSE_HTML(404), !headers_only);
}

static void ICACHE_FLASH_ATTR parse_POST(struct tcp_connection *connection)
{
}

/**
 * @brief Parse headers of a request.
 */
static void ICACHE_FLASH_ATTR parse_header(struct tcp_connection *connection,
                                           unsigned int header_offset)
{
    //Counter to keep track of where we are.
    unsigned int i = header_offset;
    //Where a single header pointers are stored.
    struct http_header *header = NULL;
    //Array where all headers are pointed to.
    struct http_header **headers;
    //Number of headers.
    unsigned short n_headers;
    //Pointer to the start of the current name or value.
    char *current_request_header;
    //True of a header name has been found.
    bool got_name = false;
    //True if something is done doing stuff.
    bool done;
    
    //Count the headers
    debug(" Counting headers");
    //We're NOT done!
    done = false;
    //Told you, see, no headers.
    n_headers = 0;
    //Go go go.
    while (!done)
    {
        if (connection->callback_data.data[i] == '\r')
        {
            //Is it the end?
            if (os_strncmp(&connection->callback_data.data[i],
                "\r\n\r\n ", 4) == 0)
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
        }
        //next character
        i++;
    }
            
    
    //Run through the response headers.
    debug(" Parsing headers:\n");
    //Go back to where the headers start.
    i = header_offset;
    //Allocate memory for the array of headers
    headers = db_zalloc(sizeof(struct http_header *) * n_headers);
    //Start from scratch.
    n_headers = 0;
    //Not done.
    done = false;
    //Save the position skipping the line feed.
    current_request_header = &connection->callback_data.data[i];
    //Go go go.
    while (!done)
    {
        if (connection->callback_data.data[i] == '\r')
        {
            //Is it the end?
            if (os_strncmp(&connection->callback_data.data[i],
                "\r\n\r\n ", 4) == 0)
            {
                debug(" Last header.\n");
                //Get out.
                done = true;
            }
            
            //End the string
            connection->callback_data.data[i] = '\0';
            debug(" Value: %s\n", current_request_header);

            /*The compiler complains that header might be uninitialised, and it
             *is right, except it *should* never happen, but lets handle it
             *anyway.
             */
            if ((got_name) && (header != NULL))
            {
                header->value = current_request_header;
                //Add the header to the array
                headers[n_headers] = header;
                //Next position.
                n_headers++;
            }
            else
            {
                os_printf("ERROR: Found a value without a name in the HTTP headers.\n");
            }
            //Skip newline.
            i++;
            current_request_header = &connection->callback_data.data[++i];
            got_name = false;
        }
        else if ((connection->callback_data.data[i] == ':') &&
                 !got_name)
        {
            //End the name at the ":"
            connection->callback_data.data[i] = '\0';
            debug(" Name %s.\n", current_request_header);
            
            //Get some memory for the pointers.
            header = (struct http_header *)db_zalloc(sizeof(struct http_header));
            header->name = current_request_header;
            
            //Next entry.
            current_request_header = &connection->callback_data.data[++i];
            
            //Signal that we got the name.
            got_name = true;
        }
        i++;
    }
    ((struct http_request *)connection->free)->headers = headers;
    ((struct http_request *)connection->free)->n_headers = n_headers;
}

static void ICACHE_FLASH_ATTR parse_HEAD(struct tcp_connection *connection)
{
    struct http_request *request = connection->free;
    unsigned int i;
    unsigned char request_entry = 0;
    char *request_data[HTTP_REQUEST_ENTRIES];
    
    //Set all pointers to NULL (kills a compiler warning).
    for (i = 0; i < HTTP_REQUEST_ENTRIES; request_data[i++] = NULL);

    /* I'm sorry about the following code, but all in all it looks much nicer,
     * than doing the same for each struct member, unrolled. 
     */
         
    //Parse the whole request line.
    debug("Parsing request line:\n");
    //Start after the method.
    i = request->start_offset;
    //Save the pointer to the start of the data (URI)
    request_data[request_entry++] = connection->callback_data.data + i;
    while (connection->callback_data.data[i] != '\r')
    {
        /* To save space, keep the original data, but replace all spaces, separating
         * data, with a zero byte, so that we can just point at each entity.
         */            
        i++;
        if (connection->callback_data.data[i] == ' ')
        {
            //Don't go beyond the entries we expect
            if (request_entry == HTTP_REQUEST_ENTRIES)
            {
                os_printf("ERROR: Request line contains to many entries.");
                break;
            }
            //Seperate the enries.
            connection->callback_data.data[i] = '\0';
            //Save a pointer to the current entry.
            request_data[request_entry++] = &connection->callback_data.data[++i];
            
            //Eat spaces to be tolerant, like spec says.
            for(; connection->callback_data.data[i] == ' ';
                connection->callback_data.data[i++] = '\0');
        }
    }
    //End the last string.
    connection->callback_data.data[i] = '\0';
    
    //Save URI
    request->uri = request_data[0];
    debug(" URI: %s\n", request->uri);  
    
    //Skip 'HTTP/' and save version.
    request->version = &request_data[1][5];
    debug(" Version: %s\n", request->version);
    
    i += 2;
    debug("Parsing headers:\n");
    parse_header(connection, i);
}

static void ICACHE_FLASH_ATTR tcp_recv_cb(struct tcp_connection *connection)
{
    struct http_request *request = connection->free;
    
    debug("HTTP received (%p).\n", connection);
    if ((connection->callback_data.data == NULL) || 
        (os_strlen(connection->callback_data.data) == 0))
    {
        os_printf("ERROR: Emtpy request recieved.\n");
        send_response(connection, HTTP_RESPONSE(400), HTTP_RESPONSE_HTML(400), true);
        return;
    }
    //Parse the request header
	if (os_strncmp(connection->callback_data.data, "GET ", 4) == 0)
    {
        debug("GET request.\n");
        request->start_offset = 4;
        request->type = GET;
        parse_HEAD(connection);
		handle_GET(connection, false);
	}
    else if(os_strncmp(connection->callback_data.data, "POST ", 5) == 0)
    {
        debug("POST request.\n");
        request->start_offset = 5;
        request->type = POST;
        parse_HEAD(connection);
		parse_POST(connection);
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501), true);
	}
    else if(os_strncmp(connection->callback_data.data, "HEAD ", 5) == 0)
    {
        debug("HEAD request.\n");
        request->start_offset = 5;
        request->type = HEAD;
        parse_HEAD(connection);
		handle_GET(connection, true);
	}
    else if(os_strncmp(connection->callback_data.data, "PUT ", 4) == 0)
    {
        debug("PUT request.\n");
        request->type = PUT;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501), true);      
	}
    else if(os_strncmp(connection->callback_data.data, "DELETE ", 7) == 0)
    {
        debug("DELETE request.\n");
        request->type = DELETE;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501), true);
	}
    else if(os_strncmp(connection->callback_data.data, "TRACE ", 6) == 0)
    {
        debug("TRACE request.\n");
        request->type = TRACE;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501), true);
	}
    else if(os_strncmp(connection->callback_data.data, "CONECT ", 5) == 0)
    {
        debug("CONNECT request.\n");
        request->type = CONNECT;
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501), true);
	}
    else
    {
        os_printf("ERROR: Unknown request: %s\n", connection->callback_data.data);
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501), true);
    }
    tcp_disconnect(connection);
}

static void ICACHE_FLASH_ATTR tcp_sent_cb(struct tcp_connection *connection )
{
}

/**
 * @brief Initialise the HTTP server,
 */
void ICACHE_FLASH_ATTR init_http(struct http_builtin_uri *builtin_uris, unsigned short n_builtin_uris)
{
    n_static_uris = n_builtin_uris;
    static_uris = builtin_uris;
    
    //Initialise TCP and listen on port 80.
    init_tcp();
    tcp_listen(80, tcp_connect_cb, tcp_reconnect_cb, tcp_disconnect_cb, 
             tcp_write_finish_cb, tcp_recv_cb, tcp_sent_cb);
}
