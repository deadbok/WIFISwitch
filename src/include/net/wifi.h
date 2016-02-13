/** wifi.h
 *
 * @brief Routines for connecting th ESP8266 to a WIFI network.
 *
 * @copyright
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
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
#ifndef WIFI_H
#define WIFI_H

/**
 * @brief Intitialise WIFI connection, by waiting for auto
 * connection to succeed.
 * 
 * @return `true` is WiFi is initialised, `false` firmware needs a restart to set WiFi mode.
 */
extern bool wifi_init(void);
/**
 * @brief Check if we're connected to a WIFI.
 *
 * Both station mode with an IP address and AP mode count as connected.
 * 
 * @return `true` on connection.
 */
extern bool wifi_check_connection(void);

/**
 * @brief Disconnect from an AP.
 *
 * @return `true` on success.
 */
extern bool wifi_disconnect(void);

#endif //WIFI_H
