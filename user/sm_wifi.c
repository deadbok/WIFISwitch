/**
 * @file sm_wifi.c
 *
 * @brief WIFI code for the main state machine.
 *
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
#include "user_interface.h"
#include "net/wifi_connect.h"

/**
 * @brief Connect to the configured Access Point, or create an AP, if unsuccessful.
 * 
 * This will keep calling itself through the main state machine until a
 * connection is made, or it times out, and sets up an access point.
 */
state_t ICACHE_FLASH_ATTR wifi_connect(void *context)
{
	static bool connected = false;
	static unsigned char state = 0;
	static bool ap_mode = false;
    bool ret = false;
    
    if (connected)
    {
		if (ap_mode)
		{
			return(WIFI_CONFIG);
		}
		else
		{
			return(WIFI_CONNECTED);
		}
	}
    
	switch (state)
	{
		case 0: ret = setup_station(&connected);
				ap_mode = false;
				break;
		case 1: ret = check_connect(&connected);
				break;
		case 2: ret = setup_ap(&connected);
				ap_mode = true;
				break;
	}
	
	if (ret)
	{
		state++;
	}
	return(WIFI_CONNECT);
}

/**
 * @brief Check if there is a connections or an AP has been started.
 */
state_t ICACHE_FLASH_ATTR wifi_connected(void *context)
{
	unsigned char ret = 0;
	unsigned char connection_status;
	
	ret = wifi_get_opmode();
	if ((ret == STATION_MODE))
	{
		//We are in client mode, check for connection to an AP.
		connection_status = wifi_station_get_connect_status();
		if (connection_status == STATION_GOT_IP)
		{
			return(WIFI_CHECK);
		}
	}
	else if (ret)
	{
		//We are in AP mode, and by definition connected.
		return(WIFI_CHECK);
	}
	return(WIFI_DISCONNECTED);
}
	
