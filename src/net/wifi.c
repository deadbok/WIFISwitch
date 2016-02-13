/**
 * @file wifi.c
 *
 * @brief Routines for connecting th ESP8266 to a WIFI network.
 *
 * - Load a WIFI configuration.
 * - If unsuccessful create an AP.
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
#include <string.h>
#include "FreeRTOS.h"
#include "espressif/esp_common.h"
#include "main.h"
#include "fwconf.h"
#include "debug.h"
#include "net/wifi.h"

/**
 * @brief Disconnect WIFI.
 * 
 * @return `true` on success.
 */
bool wifi_disconnect(void)
{
    unsigned char ret;

    //Disconnect if connected.
    ret = sdk_wifi_station_get_connect_status();
    if (ret == STATION_GOT_IP)
    {
        debug("Disconnecting.\n");
        ret = sdk_wifi_station_disconnect();
        if (!ret)
        {
            warn("Cannot disconnect (%d).", ret);
            return(false);
        }
    }
    return(true);
}

/**
 * @brief Switch to Access Point mode.
 * 
 * Switches the ESP8266 to STATIONAP mode, where both AP, and station mode is active.
 *
 * @param ssid Pointer to the SSID for the AP.
 * @param passwd Pointer to the password for the AP.
 * @param channel Channel for the AP.
 * @return true on success, false on failure.
 */
static bool wifi_apsta_set(void)
{
    struct sdk_softap_config *ap_config;
    
    debug("Setting Access Point mode.\n");
	ap_config = (struct sdk_softap_config *)db_zalloc(sizeof(struct sdk_softap_config),
													  "ap_config wifi_apsta_set");
    if (!sdk_wifi_softap_get_config(ap_config))
    {
        error("Cannot get default AP mode configuration.\n");
        return(false);
	}

    //Set station + AP mode. Station mode is needed for scanning
    //available networks.
    if (!sdk_wifi_set_opmode(STATIONAP_MODE))
    {
        error("Cannot set station + AP mode.\n");
        return(false);
    }
    debug("Switched to station + AP mode.\n");
	
	if (strnlen((const char *)ap_config->password, 64) == 0)
	{
		memcpy((char *)ap_config->password, SOFTAP_PASSWORD, strnlen(SOFTAP_PASSWORD, 64));
		printf(" Setting default AP password: %s.\n", ap_config->password);
			   
		if (!sdk_wifi_softap_set_config(ap_config))
		{
			error(" Could not set AP configuration.\n");
			return(false);
		}
	}
	debug(" Created AP SSID: %s.\n  Password: %s (%d characters).\n  "
		  "Channel %u\n  Authentication mode: %d.\n  "
		  "Hidden SSID: %d.\n  Max. connections: %d.\n  "
		  "Beacon interval: %d.\n", 
		  ap_config->ssid,
		  ap_config->password,
		  ap_config->ssid_len,
		  ap_config->channel,
		  ap_config->authmode,
		  ap_config->ssid_hidden,
		  ap_config->max_connection,
		  ap_config->beacon_interval);
	  
    return(true);
}

/**
 * @brief Connect to the configured Access Point.
 *
 */
static bool wifi_station_set(void)
{
    unsigned char ret = 0;
    struct sdk_station_config station_conf;
    
    debug("Setting to station mode.\n");
	if (!sdk_wifi_set_opmode(STATION_MODE))
	{
		error("Could not enter WiFi station mode.\n");
		return(false);
	}

	//Get a pointer the configuration data.
	ret = sdk_wifi_station_get_config(&station_conf);
	if (!ret)
	{
		error("Cannot get station configuration (%d).", ret);
		return(false);
	}

	debug(" Connecting to SSID: %s.\n  Password: %s.\n  "
		  "BSSID set: %d.\n  BSSID: %s.\n", station_conf.ssid,
		  station_conf.password,
		  station_conf.bssid_set,
		  station_conf.bssid);

	return(true);
}

bool wifi_check_connection(void)
{
	unsigned char ret = 0;
	
	debug("WiFi connection check.\n");
	ret = sdk_wifi_get_opmode();
	debug(" Operation mode: %d.\n", ret);
	if ((ret == STATION_MODE))
	{
		//We are in client mode, check for connection to an AP.
		ret = sdk_wifi_station_get_connect_status();
		debug(" Connection status: %d.\n", ret);
		if (ret == STATION_GOT_IP)
		{
			return(true);
		}
	}
	else if (ret)
	{
		//We are in AP mode, and by definition connected.
		return(true);
	}
	return(false);
}

bool wifi_init()
{
	/* This was easier in the NON-OS SDK. I aim to let esp-open-rtos
	 * do the WiFi initialisation at boot, since trying to force it
	 * has led to nothing but exceptions when switching to station+AP
	 * mode. If the mode parameter is different from the configured
	 * value, th emode is set and the firmware is restarted. */

	uint8_t mode;

	debug("WiFi init.\n");
	mode= sdk_wifi_get_opmode();
	debug(" Current operation mode: %d.\n", mode);
	debug(" Configured operation mode: %d.\n", cfg->network_mode);
	
	//Check if mode has changed.
	if ((!mode) || (mode != cfg->network_mode))
	{
		//We don't do SOFTAP mode.
		if (cfg->network_mode < SOFTAP_MODE)
		{
			wifi_station_set();
		}
		else
		{
			wifi_apsta_set();
		}
		return(false);
	}
	if (cfg->network_mode < SOFTAP_MODE)
	{
		printf("WiFi in station mode.\n");
	}
	else
	{
		printf("WiFi in Access Point mode.\n");
	}
	return(true);

	/**
	 * @todo Make this work with lwip.
	 */
	//if (wifi_station_set_hostname(hostname))
	//{
		//db_printf("Host name set to %s.\n", hostname);
	//}
	//else
	//{
		//warn("Could not set host name.\n");
	//}
}
