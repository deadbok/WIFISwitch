/* http.c
 *
 * Simple HTTP server for the ESP8266.
 * 
 * - Does GET and POST requests.
 * - Read the served files from flash
 * 
 * What works:
 * - Almost nothing.
 * 
 * THIS SERVER IS NOT COMPLIANT WITH HTTP, TO SAVE SPACE, STUFF HAS BEEN
 * OMITTED.
 * 
 * Missing functionality:
 * - Most header field.
 * - 
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
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
#include <stdlib.h>
#include "missing_dec.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "tcp.h"
#include "http.h"

//Number of entries in the HTTP request line, excluding the method.
#define REQUEST_ENTRIES     2

//Request methods
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

struct http_request
{
    enum request_type   type;
    unsigned char       start_offset;
    char                *uri;
    char                *version;
    char                *message;
};

static struct http_request current_request;
static struct http_builtin_uri *static_uris;
static unsigned short n_static_uris;

void ICACHE_FLASH_ATTR tcp_connect_cb(void)
{
}

void ICACHE_FLASH_ATTR tcp_reconnect_cb(void)
{
}

void ICACHE_FLASH_ATTR tcp_disconnect_cb(void)
{
}

void ICACHE_FLASH_ATTR tcp_write_finish_cb(void)
{
}

static void ICACHE_FLASH_ATTR parseGET(void)
{
    char    *response;
    char    *html;
    char    html_length[6];
    unsigned short  i;
    
    //Check in static uris, go through and stop if URIs match.
    for (i = 0; 
         ((i < n_static_uris) && (os_strcmp(current_request.uri, static_uris[i].uri) != 0)); 
         i++);
    
    //Send response if URI is found
    if (i < n_static_uris)
    {
        response = HTTP_200_OK;
        
        html = static_uris[i].callback(current_request.uri);
    }
    else
    {
        //Send 404
        response = HTTP_404_NOT_FOUND;
        
        html = HTTP_404_NOT_FOUND_HTML;
    }
    tcp_send(response);
    os_sprintf(html_length, "%d", os_strlen(html));
    tcp_send(html_length);
    tcp_send("\r\n\r\n");
    tcp_send(html);
}

static void ICACHE_FLASH_ATTR parsePOST(void)
{
}

static void ICACHE_FLASH_ATTR parseHEAD(void)
{
    unsigned int    i;
    unsigned char   request_entry = 0;
    char            *request_data[REQUEST_ENTRIES];
    char            *current_request_header;
    
    //Set all pointers to NULL (kills a compiler warning).
    for (i = 0; i < REQUEST_ENTRIES; request_data[i++] = NULL);

    /* I'm sorry about the following code, but all in all it looks much nicer,
     * than doing the same for each struct member, unrolled. 
     */
         
    //Parse the whole request line.
    debug("Parsing request line:\n");
    //Start after the method.
    i = current_request.start_offset;
    //Save the pointer to the start of the data (URI)
    request_data[request_entry++] = tcp_cb_data.data + i;
    while (tcp_cb_data.data[i] != '\r')
    {
        /* To save space, keep the original data, but replace all spaces, separating
         * data, with a zero byte, so that we can just point at each entity.
         */            
        i++;
        if (tcp_cb_data.data[i] == ' ')
        {
            //Don't go beyond the entries we expect
            if (request_entry == REQUEST_ENTRIES)
            {
                os_printf("Request line contains to many entries.");
                break;
            }
            //Seperate the enries.
            tcp_cb_data.data[i] = '\0';
            //Save a pointer to the current entry.
            request_data[request_entry++] = &tcp_cb_data.data[++i];
            
            //Eat spaces to be tolerant, like spec says.
            for(; tcp_cb_data.data[i] == ' '; tcp_cb_data.data[i++] = '\0');
        }
    }
    //End the last string.
    tcp_cb_data.data[i] = '\0';
    
    //Save URI
    current_request.uri = request_data[0];
    debug(" URI: %s\n", current_request.uri);  
    
    //Skip 'HTTP/' and save version.
    current_request.version = &request_data[1][5];
    debug(" Version: %s\n", current_request.version);
    
    //Run through the response headers.
    debug("Parsing headers:\n");
    //Save the position skipping the line feed.
    i++;
    current_request_header = &tcp_cb_data.data[++i];
    while(tcp_cb_data.data[i] != '\0')
    {
        if (tcp_cb_data.data[i] == '\r')
        {
#ifdef DEBUG
            tcp_cb_data.data[i] = '\0';
            debug(" %s\n", current_request_header);
#endif
            //Tell that we re not spying on a Do Not Track header.
            if (os_strncmp(current_request_header, "DNT: 1", 6) == 0)
            {
                os_printf("I am no spy. NSA connection closed.\n");
            }
            //Save the position skipping the line feed.
            i++;
            current_request_header = &tcp_cb_data.data[++i];
        }
        i++;
    }
}

void ICACHE_FLASH_ATTR tcp_recv_cb(void)
{
    if ((tcp_cb_data.data == NULL) || (os_strlen(tcp_cb_data.data) == 0))
    {
        os_printf("Emtpy request recieved.\n");
        return;
    }
    //Parse the request header
	if (os_strncmp(tcp_cb_data.data, "GET ", 4) == 0)
    {
        debug("GET request.\n");
        current_request.start_offset = 4;
        parseHEAD();
		parseGET();
	}
    else if(os_strncmp(tcp_cb_data.data, "POST ", 5) == 0)
    {
        debug("POST request.\n");
        current_request.start_offset = 5;
        parseHEAD();
		parsePOST();
	}
    else if(os_strncmp(tcp_cb_data.data, "HEAD ", 5) == 0)
    {
        debug("HEAD request.\n");
        current_request.start_offset = 5;
        parseHEAD();
	}
    else if(os_strncmp(tcp_cb_data.data, "PUT ", 4) == 0)
    {
        debug("PUT request.\n");        
	}
    else if(os_strncmp(tcp_cb_data.data, "DELETE ", 7) == 0)
    {
        debug("DELETE request.\n");        
	}
    else if(os_strncmp(tcp_cb_data.data, "TRACE ", 6) == 0)
    {
        debug("TRACE request.\n");        
	}
    else if(os_strncmp(tcp_cb_data.data, "CONECT ", 5) == 0)
    {
        debug("CONNECT request.\n");        
	}
    else
    {
        os_printf("Unknown request: %s\n", tcp_cb_data.data);
    }
        
    /*char    *response = "HTTP/1.0 200 OK\r\nContent-Type: text/html\
                         \r\nConnection: close\r\nContent-Length: ";
    char    *html = "<!DOCTYPE html><head><title>Web server test.</title></head>\
                     <body>Hello world.</body></html>";
    char    html_length[6];
    
    if(os_strncmp(tcp_cb_data.data,"GET ",4) && os_strncmp(tcp_cb_data.data,"get ",4))
    {
        os_printf("Unknown request.\n");
    }
    else
    {
        debug("GET request.\n");
        tcp_send(response);
        os_sprintf(html_length, "%d", os_strlen(html));
        tcp_send(html_length);
        tcp_send("\r\n\r\n");
        tcp_send(html);
    }*/
    tcp_disconnect();
}

void ICACHE_FLASH_ATTR tcp_sent_cb(void)
{
}

void ICACHE_FLASH_ATTR init_http(struct http_builtin_uri *builtin_uris, unsigned short n_builtin_uris)
{
    n_static_uris = n_builtin_uris;
    static_uris = builtin_uris;
    
    init_tcp(80, tcp_connect_cb, tcp_reconnect_cb, tcp_disconnect_cb, 
             tcp_write_finish_cb, tcp_recv_cb, tcp_sent_cb);
}
