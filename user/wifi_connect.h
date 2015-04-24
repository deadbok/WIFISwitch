/* wifi_connect.h
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
 
//The current station config.
extern struct station_config station_conf;
//The current AP configuration.
extern struct softap_config  ap_config;
//Connection status.
extern unsigned char         connect_status;

//Connect to WIFI.
extern void wifi_connect(void (*connect_cb)());

#endif
