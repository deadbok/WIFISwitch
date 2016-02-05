
/**
 * @file wifiswitch.h
 *
 * @brief Websocket wifiswitch protocol code.
 * 
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
#ifndef WIFISWITCH_H
#define WIFISWITCH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "user_config.h"
#include "tools/jsmn.h"
#include "net/websocket.h"

#define WS_PR_WIFISWITCH "wifiswitch"

#ifndef WS_WIFISWITCH_TIMEOUT
#warning "No connection time out set for WebSocket protocol wifiswitch."
/**
 * @brief Connection time out in milliseconds.
 */
#define WS_WIFISWITCH_TIMEOUT 600000
#endif

#ifndef WS_WIFISWITCH_GPIO_ENABLED
#warning "No GPIO's has been enabled for access via the wifiswitch protocol."
/**
 * @brief Bit mask of which GPIO pins the WebSocket API can control.
 */
#define WS_WIFISWITCH_GPIO_ENABLED 0
#endif

/**
 * @brief Number of GPIO's
 */
#define WS_WIFISWITCH_GPIO_PINS 16

/**
 * @brief Handler registration structure.
 */
extern struct ws_handler ws_wifiswitch_handler;

/**
 * @brief Register handler with WebSocket protocol.
 */
extern bool ws_register_wifiswitch(void);
extern signed long int ws_wifiswitch_received(struct ws_frame *frame, struct net_connection *connection);
extern signed long int ws_wifiswitch_close(struct ws_frame *frame, struct net_connection *connection);

/**
 * @brief Send GPIO status to all clients.
 */
extern void ws_wifiswitch_send_gpio_status(void);

#endif //WIFISWITCH_H
