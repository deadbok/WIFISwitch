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
 * - POST requests.
 * - CRLF, LF, and space tolerance (never tested except for space).
 * 
 * **This server is not compliant with HTTP, to save space, stuff has been
 * omitted. It has been built to be small, not fast, and probably breaks in 100
 * places.**
 * 
 * Missing functionality:
 * - Does not understand any header fields.
 * - MIME types.
 * - File system access.
 * - 400 errors are not send in all situations where they should be.
 * - Persistent connections. The server closes every connection when the 
 *   response has been sent.
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
#include "c_types.h"
#include "osapi.h"
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "tcp.h"
#include "strxtra.h"
#include "http.h"

/**
 * @brief Eat initial spaces.
 * 
 * Eat initial spaces in a string replacing then with '\0', and advance the pointer.
 */
#define HTTP_EAT_SPACES(string) while (*string == ' ')\
                                    *string++ = '\0'
/**
 * @brief eat line ends from HTTP headers.
 * 
 * This macro is used to be tolerant, accepting both CRLF and LF endings.
 */
#define HTTP_EAT_CRLF(ptr, number)  do\
                                    {\
                                        if (*ptr == '\r')\
                                        {\
                                            os_bzero(ptr, number * 2);\
                                            ptr += (number * 2);\
                                        }\
                                        else\
                                        {\
                                            os_bzero(ptr, number);\
                                            ptr += number;\
                                        }\
                                    } while(0)
/* Wrapping stuff in do...while(0), makes it possible to invoke the macro like a
 * function call ending with semicolon (;).
 * Idea from here: [https://gcc.gnu.org/onlinedocs/cpp/Swallowing-the-Semicolon.html
 */

/**
 * @brief Built in pages.
 * 
 * Built in pages that are served when every other method of getting the resource
 * has failed.
 */
static struct http_builtin_uri *static_uris;
/**
 * @brief Number of built in pages.
 */
static unsigned short n_static_uris;

/**
 * @brief Test if a header is set to a certain value.
 * 
 * @param request HTTP request to check.
 * @param name Name of the header. *Must be lower case.*
 * @param value Value to test for. *Case sensitive.*
 * @return `true`if found, `false` otherwise.
 */
bool ICACHE_FLASH_ATTR is_header_value(struct http_request *request, char *name, char *value)
{
    unsigned short i;
    char *current_name;
    char *current_value;
    
    for (i = 00; i < request->n_headers; i++)
    {
        current_name = request->headers[i]->name;
        current_value = request->headers[i]->value;
        if ((os_strcmp(current_name, name) == 0) && 
            (os_strcmp(current_value, value) == 0))
        {
            return(true);
        }
    }
    return(false);
}

/**
 * @brieef Print a Common Log Format message to the console.
 * 
 * Log access to the web server to console in a standardized text file format:
 * 
 *  host ident authuser date request status bytes
 * 
 * or:
 * 
 *  127.0.0.1 user-identifier frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326
 * 
 * See [CLF](https://en.wikipedia.org/wiki/Common_Log_Format).
 */
static void ICACHE_FLASH_ATTR print_clf_status(struct tcp_connection *connection,
                                               char *status_code, char *length)
{
    struct http_request *request = connection->free;

    char *unknown = "-";
    
    if (connection)
    {
        os_printf(IPSTR " %s %s %s %s HTTP/%s %s %s\n",
                  IP2STR(connection->conn->proto.tcp->remote_ip), unknown,
                  unknown, unknown, connection->callback_data.data,
                  request->version, status_code, length);
    }
}

/**
 * @brief Callback when a connection is made.
 * 
 * Called whenever a connections is made. Sets up the data structures for
 * the server.
 * 
* @param connection Pointer to the connection that has connected.
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

/**
 * @brief Called on connection error.
 * 
 * @param connection Pointer to the connection that has had an error.
 */
static void ICACHE_FLASH_ATTR tcp_reconnect_cb(struct tcp_connection *connection)
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

/**
 * @brief Called when a write is done I guess?
 * 
 * @param connection Pointer to the connection that is finished.
 */
static void ICACHE_FLASH_ATTR tcp_write_finish_cb(struct tcp_connection *connection)
{
}

/**
 * @brief Send a HTTP response.
 * 
 * Send a HTTP response, and close the connection, if the headers says so.
 * 
 * @param connection Connection to use when sending.
 * @param response Response header (without the final `\r\n\r\n`.
 * @param content The content of the response.
 * @param send_content If `false` only send headers.
 * @param close Close the connection after the response. 
 */
static void ICACHE_FLASH_ATTR send_response(struct tcp_connection *connection,
                                                   char *response,
                                                   char *content,
                                                   bool send_content,
                                                   bool close)
{
    char *h_connection_close = "Connection: close\r\n";
    char *h_length = "Content-Length: ";
    char status_code[4] = "000\0";
    char html_length[10];
    char *s_status_code;
 
    //Send start of the response.
    tcp_send(connection, response);
    //Send close announcement if needed.
    if (close)
    {
        tcp_send(connection, h_connection_close);
    }

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
    //Find status code and skip spaces.
    for (s_status_code = os_strstr(response, " "); *s_status_code == ' '; s_status_code++);
    if (s_status_code)
    {
        os_memcpy(status_code, s_status_code, 3);
        print_clf_status(connection, status_code, html_length);
    }
    else
    {
        print_clf_status(connection, "-", html_length);
    }
    if (close)
    {
        tcp_disconnect(connection);
    }
}

/**
 * @brief Handle a GET request.
 * 
 * @param connection Pointer to the connection used.
 * @param headers_only Send only headers, make this usable for HEAD requests
 *                     as well.
 */
static void ICACHE_FLASH_ATTR handle_GET(struct tcp_connection *connection, bool headers_only)
{
    struct http_request *request = connection->free;
    unsigned short  i;
    
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
static char ICACHE_FLASH_ATTR *parse_header(struct tcp_connection *connection,
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
                os_printf("ERROR: Could not parse request header: %s\n", data);
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
static char ICACHE_FLASH_ATTR *parse_HEAD(struct tcp_connection *connection, 
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
        os_printf("ERROR: Could not parse HTTP request URI (%s).\n", request_entry);
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
        os_printf("ERROR: Could not parse HTTP request version (%s).\n", request_entry);
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

/**
 * @brief Called when data has been received.
 * 
 * Parse the HTTP request, and send a meaningful answer.
 * 
 * @param connection Pointer to the connection that received the data.
 */
static void ICACHE_FLASH_ATTR tcp_recv_cb(struct tcp_connection *connection)
{
    struct http_request *request = connection->free;
    
    debug("HTTP received (%p).\n", connection);
    if ((connection->callback_data.data == NULL) || 
        (os_strlen(connection->callback_data.data) == 0))
    {
        os_printf("ERROR: Emtpy request recieved.\n");
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
        os_printf("ERROR: Unknown request: %s\n", connection->callback_data.data);
        send_response(connection, HTTP_RESPONSE(501), HTTP_RESPONSE_HTML(501),
                      true, HTTP_CLOSE_CONNECTIONS);
    }
}

static void ICACHE_FLASH_ATTR tcp_sent_cb(struct tcp_connection *connection )
{
}

/**
 * @brief Initialise the HTTP server,
 * 
 * @param builtin_uris An array of built in handlers for URIs.
 * @param n_builtin_uris Number of URI handlers.
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