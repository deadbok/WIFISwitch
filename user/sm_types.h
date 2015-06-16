/** @file sm_types.h
 *
 * @brief Main state machine type definitions.
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
#ifndef SM_TYPES_H
#define SM_TYPES_H

/**
 * @brief Type of a state.
 */
typedef unsigned int state_t;

/**
 * @brief Type for the event handler functions.
 */
typedef state_t (*state_handler_t)(void *);

/**
 * @brief Represents all states of the running firmware.
 * 
 * Positions has to correspond with positions in #handlers (in sm.h).
 */
enum states
{
	/**
	 * @brief Connect to the network or create an access point.
	 * 
	 * Jumps to itself (WIFI_CONNECT), until a connection has been made
	 * (WIFI_CONNECTED), or an access point has been created (WIFI_CONFIG)
	 */
	WIFI_CONNECT,
	/**
	 * @brief Check network connection..
	 * 
	 * Jumps to HTTP_INIT on connection, and WIFI_DISCONNECTED on no connection.
	 */
	WIFI_CHECK,
	/**
	 * @brief Set root of the server for normal operation and continue to
	 * HTTP_INIT.
	 */
	WIFI_CONNECTED,
	/**
	 * @brief Set root of the server for configuration operation and continue
	 * to HTTP_INIT.
	 */
	WIFI_CONFIG,
	WIFI_DISCONNECTED,
	/**
	 * @brief Initialise the HTTP server, and move on to HTTP_SEND or 
	 * SYSTEM_RESTART if the server couldn't start.
	 */
	HTTP_INIT,
	/**
	 * @brief Send some data, if a request is waiting, and move on to
	 * HTTP_CLEANUP.
	 */
	HTTP_SEND,
	HTTP_CLEANUP,
	SYSTEM_RESTART
};

/**
 * @brief Main state machine context.
 */
struct sm_context
{
	/**
	 * @brief Root of the HTTP server.
	 */
	 char *http_root;
};

#endif //SM_TYPES_H
