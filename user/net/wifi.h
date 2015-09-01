/* wifi.h
 *
 * Routines for connecting th ESP8266 to a WIFI network.
 *
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
#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

/**
 * @brief No connection status.
 */
#define WIFI_MODE_NO_CONNECTION 0
/**
 * @brief Connected to the configured AP.
 */
#define WIFI_MODE_CLIENT 1
/**
 * @brief In configuration mode acting as client and AP.
 */
#define WIFI_MODE_CONFIG 2

extern bool wifi_init(char *hostname, 
					  void (*connect_cb)(unsigned char wifi_mode),
					  void (*disconnect_cb)());
extern bool wifi_check_connection(void);

#endif
