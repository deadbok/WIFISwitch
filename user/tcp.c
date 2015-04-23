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

static void ICACHE_FLASH_ATTR tcp_connect_cb(void *arg)
{
    debug("TCP connection.\n");
}

void ICACHE_FLASH_ATTR init_tcp(int port)
{
    static struct espconn tcp_conn;
    static esp_tcp tcp;
    
    debug("TCP init.\n");
    

    tcp_conn.type = ESPCONN_TCP;
    tcp_conn.state = ESPCONN_NONE;
    tcp.local_port = port;
    tcp_conn.proto.tcp = &tcp;
    
    espconn_regist_connectcb(&tcp_conn, tcp_connect_cb);
    espconn_accept(&tcp_conn);  
}
