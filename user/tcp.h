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
#ifndef TCP_H
#define TCP_H

#include "espconn.h"
#include "dl_list.h"

struct tcp_callback_data
{
    void            *arg;
    char            *data;
    unsigned short  length;
    err_t            err;
};

struct tcp_connection;
typedef void (*tcp_callback)(struct tcp_connection *);

struct tcp_callback_funcs
{
    tcp_callback connect_callback;
    tcp_callback reconnect_callback;
    tcp_callback disconnect_callback;
	tcp_callback write_finish_fn;
    tcp_callback recv_callback;
    tcp_callback sent_callback;
};

struct tcp_connection
{
    struct espconn              *conn;
    struct tcp_callback_funcs   callbacks;
    struct tcp_callback_data    callback_data;
    DL_LIST_CREATE(struct tcp_connection);
};
    

extern struct tcp_callback_data     tcp_cb_data;
extern struct tcp_callback_funcs    tcp_cb_funcs;

struct tcp_connection *tcp_listen(int port, tcp_callback connect_cb, 
                                tcp_callback reconnect_cb, 
                                tcp_callback disconnect_cb, 
                                tcp_callback write_finish_cb, 
                                tcp_callback recv_cb, 
                                tcp_callback sent_cb);
char tcp_send(struct tcp_connection *connection, char *data);
char tcp_disconnect(struct tcp_connection *connection);
void init_tcp(void);

#endif
