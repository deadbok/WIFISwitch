/* tcp.c
 *
 * TCP Routines for the ESP8266.
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
#include "missing_dec.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"

//Active connection.
static struct espconn tcp_conn;
static esp_tcp tcp;

//Internal function to print espconn errors.
static void print_status(int status)
{
    switch(status)
    {
        case ESPCONN_OK:
            debug("OK.\n");
            break;
        case ESPCONN_MEM:
            os_printf("Out of memory.\n");
            break;
        case ESPCONN_TIMEOUT:
            os_printf("Timeout.\n");
            break;            
        case ESPCONN_RTE:
            os_printf("Routing problem.\n");
            break;
        case ESPCONN_INPROGRESS:
            os_printf("Operation in progress...\n");
            break;
        case ESPCONN_ABRT:
            os_printf("Connection aborted.\n");
            break;
        case ESPCONN_RST:
            os_printf("Connection reset.\n");
            break;
        case ESPCONN_CLSD:
            os_printf("Connection closed.\n");
            break;
        case ESPCONN_CONN:
            os_printf("Not connected.\n");
            break;
        case ESPCONN_ARG:
            os_printf("Illegal argument.\n");
            break;
        case ESPCONN_ISCONN:
            os_printf("Already connected.\n");
            break;
        default:
            os_printf("Unknown status: %d\n", status);
            break;    
    }
}

static void ICACHE_FLASH_ATTR tcp_recv_cb(void *arg, char *data, unsigned short length)
{
    char    *response = "HTTP/1.0 200 OK\r\nContent-Type: text/html\
                         \r\nContent-Length: ";
    char    *html = "<!DOCTYPE html><head><title>Web server test.</title></head>\
                     <body>Hello world.</body></html>";
    char    html_length[6];
    
    debug("TCP recieved:\n");
    debug("%s\n", data);

    if(os_strncmp(data,"GET ",4) && os_strncmp(data,"get ",4))
    {
        os_printf("Unknown request.\n");
    }
    else
    {
        debug("GET request.\n");
        espconn_sent(&tcp_conn, (unsigned char *)response, os_strlen(response));
        os_sprintf(html_length, "%d", os_strlen(html));
        espconn_sent(&tcp_conn, (unsigned char *)html_length, os_strlen(html_length));
        espconn_sent(&tcp_conn, (unsigned char *)"\r\n\r\n", 4);
        espconn_sent(&tcp_conn, (unsigned char *)html, os_strlen(html));
    }
}

void ICACHE_FLASH_ATTR init_tcp(int port)
{
    int ret;
    
    debug("TCP init.\n");
    
    //TCP Setup
    //Port to bind to.
    tcp.local_port = port;
    
    //Setup TCP connection configuration.
    //TCP connection
    tcp_conn.type = ESPCONN_TCP;
    //No state.
    tcp_conn.state = ESPCONN_NONE;
    //TCP setup
    tcp_conn.proto.tcp = &tcp;
    //
    tcp_conn.recv_callback = tcp_recv_cb;
    
    debug("Accepting connections on port %d...", port);
    ret = espconn_accept(&tcp_conn);
    print_status(ret); 
}
