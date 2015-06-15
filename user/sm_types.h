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
 */
enum states
{
	WIFI_CONNECT,
	WIFI_CHECK,
	WIFI_CONNECTED,
	WIFI_CONFIG,
	WIFI_DISCONNECTED,
	HTTP_INIT
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
