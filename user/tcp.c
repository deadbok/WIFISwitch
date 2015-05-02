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
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "tcp.h"

//Active connection.
static struct espconn tcp_conn;
static esp_tcp tcp;

//TCP data & callbacks
struct tcp_callback_data    tcp_cb_data;
struct tcp_callback_funcs   tcp_cb_funcs;

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

static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg)
{
    debug("TCP connected.\n");
    
    //Clear previous data.
    os_memset(&tcp_cb_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    tcp_cb_data.arg = arg;
}

static void ICACHE_FLASH_ATTR tcp_reconnect_cb(void *arg, sint8 err)
{
    debug("TCP reconnected.\n");
    
    //Clear previous data.
    os_memset(&tcp_cb_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    tcp_cb_data.arg = arg;
    tcp_cb_data.err = err;
}

static void ICACHE_FLASH_ATTR tcp_disconnect_cb(void *arg)
{
    debug("TCP disconnected.\n");
    
    //Clear previous data.
    os_memset(&tcp_cb_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    tcp_cb_data.arg = arg;
}

static void ICACHE_FLASH_ATTR tcp_write_finish_cb(void *arg)
{
    debug("TCP write done.\n");

    //Clear previous data.
    os_memset(&tcp_cb_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    tcp_cb_data.arg = arg;
}

static void ICACHE_FLASH_ATTR tcp_recv_cb(void *arg, char *data, unsigned short length)
{
    debug("TCP recieved:\n");
    debug("%s\n", data);
    
    //Clear previous data.
    os_memset(&tcp_cb_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    tcp_cb_data.arg = arg;
    tcp_cb_data.data = data;
    tcp_cb_data.length = length;
    
    tcp_cb_funcs.recv_callback();
}

static void ICACHE_FLASH_ATTR tcp_sent_cb(void *arg)
{
    debug("TCP sent.\n");

    //Clear previous data.
    os_memset(&tcp_cb_data, 0, sizeof(struct tcp_callback_data));
    //Set new data
    tcp_cb_data.arg = arg;
}

/* Send TCP data to the current connection.
 * 
 * Parameters:
 *  data
 *   Pointer to the data to send
 * 
 * Returns:
 *  espconn error status
 */
char ICACHE_FLASH_ATTR tcp_send(char *data)
{
    debug("Sending TCP: %s\n", data);
    return(espconn_sent(&tcp_conn, (unsigned char *)data, os_strlen(data)));
}

/* Disconnect the current TCP connection.
 * 
 * Parameters:
 *  none
 * 
 * Returns:
 *  espconn error status
 */
char ICACHE_FLASH_ATTR tcp_disconnect(void)
{
    return(espconn_disconnect(&tcp_conn));
}


/* Initialise TCP networking.
 * 
 * Parameters:
 *  port
 *   The TCP port to bind to.
 * 
 * Returns:
 *  none
 */
void ICACHE_FLASH_ATTR init_tcp(int port, tcp_callback connect_cb, 
                                tcp_callback reconnect_cb, tcp_callback disconnect_cb,
                                tcp_callback write_finish_cb, tcp_callback recv_cb,
                                tcp_callback sent_cb)
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
    
    //Setup internal callbacks
    tcp.connect_callback = tcp_connect_cb;
    tcp.reconnect_callback = tcp_reconnect_cb;
    tcp.disconnect_callback = tcp_disconnect_cb;
	tcp.write_finish_fn = tcp_write_finish_cb;
    tcp_conn.recv_callback = tcp_recv_cb;
    tcp_conn.sent_callback = tcp_sent_cb;

    //Setup user callbacks
    tcp_cb_funcs.connect_callback = connect_cb;
    tcp_cb_funcs.reconnect_callback = reconnect_cb;
    tcp_cb_funcs.disconnect_callback = disconnect_cb;
    tcp_cb_funcs.write_finish_fn = write_finish_cb;
    tcp_cb_funcs.recv_callback = recv_cb;
    tcp_cb_funcs.sent_callback = sent_cb;
    
    debug("Accepting connections on port %d...", port);
    ret = espconn_accept(&tcp_conn);
    print_status(ret); 
}
