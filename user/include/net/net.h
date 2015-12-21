/** @file net.h
 *
 * @brief General network related interface.
 * 
 * Buffering for waiting send data. The SDK will fault, when trying to
 * send something before the last data has been send, and the sent
 * callback, has been invoked. This manages a ring buffer of
 * #NET_MAX_SEND_QUEUE items waiting to be send.
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
#ifndef HTTP_NET_H
#define HTTP_NET_H

#include "c_types.h"
#include "os_type.h"
//Should be included by espconn.h
#include "ip_addr.h"
#include "espconn.h"

/**
 * @brief Ring buffer for connections waiting to send data.
 */
#define NET_MAX_SEND_QUEUE 20

/**
 * @brief Send or buffer some data.
 * 
 * @param data Pointer to data to send.
 * @param len Length of data to send (in bytes).
 * @param connection SDK connection to use.
 * @return Bytes send of buffered.
 */
size_t net_send(char *data, size_t len, struct espconn *connection);
/**
 * @brief Send an item in the send buffer.
 * 
 * Include this in any network sent callback to send the next item
 * waiting.
 */
void net_sent_callback(void);

/**
 * @brief Tell if there is stuff being send.
 * 
 * @return `True` if sending, `false` otherwise.
 */
bool net_is_sending(void);

#endif
