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
#include "os_type.h"
#include "user_interface.h"
#include "osapi.h"
#include "user_config.h"
#include "tools/missing_dec.h"
#include "tools/strxtra.h"
#include "net/wifi.h"

static os_signal_t connected_signal;
static os_signal_t disconnected_signal;

/**
 * @brief Connection mode.
 * 
 * Values:
 *  0 No connection.
 *  1 Normal.
 *  2 AP.
 */
static unsigned char wifi_connected = WIFI_MODE_NO_CONNECTION;

//Retry counter.
static unsigned char timeout = CONNECT_DELAY_SEC;
//Timer for waiting for connection.
static os_timer_t connected_timer;

/**
 * @brief Disconnect WIFI.
 * 
 * @return `true` on success.
 */
static bool ICACHE_FLASH_ATTR wifi_disconnect(void)
{
    unsigned char ret;

    //Disconnect if connected.
    ret = wifi_station_get_connect_status();
    if (ret == STATION_GOT_IP)
    {
        debug("Disconnecting.\n");
        ret = wifi_station_disconnect();
        if (!ret)
        {
            warn("Cannot disconnect (%d).", ret);
            return(false);
        }
        ret = wifi_station_dhcpc_stop();
        if (!ret)
        {
            warn("Cannot stop DHCP client (%d).", ret);
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
static bool create_softap(char *ssid, char *passwd, unsigned char channel)
{
    unsigned char ret;
    struct softap_config ap_config;

    debug("Creating AP SSID: %s, password: %s, channel %u\n", ssid, passwd, 
          channel);

    //Set station + AP mode. Station mode is needed for scanning available networks.
    if (!wifi_set_opmode(STATIONAP_MODE))
    {
        error("Cannot set station + AP mode.\n");
        return(false);
    }
    if (!wifi_softap_get_config(&ap_config))
    {
        error("Cannot get default AP mode configuration.\n");
        return(false);
	}
	
    //Populate the struct, needed to configure the AP.
    os_strcpy((char *)ap_config.ssid, ssid);
    os_strcpy((char *)ap_config.password, passwd);
    ap_config.ssid_len = os_strlen((char *)ap_config.ssid);
    ap_config.channel = channel;
    //WPA & WPA2 Pre Shared Key (e.g "home" networks).
    ap_config.authmode = AUTH_WPA2_PSK;
    ap_config.ssid_hidden = 0;
    //We don't need a lot of connections for our purposes.
    //ap_config->max_connection = 4;
    //ap_config.beacon_interval = 100;

    //Set AP configuration.
    ret = wifi_softap_set_config_current(&ap_config);
    if (!ret)
    {
        error("Failed to set soft AP configuration (%d).", ret);
        return(false);
    }
    return(true);
}

/**
 * @brief Create an AP for configuration mode.
 *
 * Creates an access point named ESP+MAC_ADDR with the password of #SOFTAP_PASSWORD.
 * 
 * @return true on success, false on failure.
 */
static bool wifi_ap_mode(void)
{
    unsigned char           mac[6];
    char                    ssid[32];
    char                    passwd[11] = SOFTAP_PASSWORD;
    unsigned char           i;
    char                    temp_str[3];
    unsigned char           ret;

    db_printf("Starting in Wifi configuration mode.\n");
    
    /* Disconnect to try and prevent the SDK from trying to reconnect to
     * the configured AP when in config mode.
     */
     wifi_disconnect();

    //Get MAC address.
    ret = wifi_get_macaddr(SOFTAP_IF, mac);
    if (!ret)
    {
        error("Cannot get MAC address (%d).", ret);
        return(false);
    }
    debug("MAC: " MACSTR "\n", MAC2STR(mac));

    //Create an AP with SSID "ESP" + MAC address, and password "1234567890".
    //Generate the SSID.
    strcpy(ssid, "ESP\0");
    //Splice in the MAC as part of the SSID.
    for (i = 0; i < 6; i++)
    {
        itoa(mac[i], temp_str, 16);
        strcat(ssid, temp_str);
    }
    //Zero terminator.
    ssid[15] = '\0';
    debug("SSID: %s\n", ssid);

    //Create the AP.
    if (!create_softap(ssid, passwd, 10))
    {
		error("Could not start configuration mode.\n");
		return(false);
	}
    
    wifi_connected = WIFI_MODE_AP;
    //Reset retry counter.
    timeout = CONNECT_DELAY_SEC;
    //Call connected task.
    task_raise_signal(connected_signal, wifi_connected);

	return(true);
}

/**
 * @brief Call back to handle WIFI events from the SDK.
 * 
 * @param event Event data from SDK.
 */
static void wifi_handle_event_cb(System_Event_t *event)
{
	switch (event->event)
	{
		//Connected in station mode.
		case     EVENT_STAMODE_CONNECTED:
			db_printf("Connected to AP %s.\n", 
					  event->event_info.connected.ssid);
			break;
		//Disconnected from AP.
		case     EVENT_STAMODE_DISCONNECTED:
			//Ignore in AP mode.
			if (wifi_connected != WIFI_MODE_AP)
			{
				debug("Disconnected from AP.\n");
				debug("Reason: %d.\n", event->event_info.disconnected.reason);
				debug("Mode: %d, time out %d second(s).\n", wifi_connected, timeout);
				//Ignore failed client attempts when disconnected and when initially running in the time out.
				if ((wifi_connected == WIFI_MODE_CLIENT) || (!timeout))
				{
					task_raise_signal(disconnected_signal, 0);
				}
				wifi_connected = WIFI_MODE_NO_CONNECTION;
			}
			break;
		//Unused.
		case     EVENT_STAMODE_AUTHMODE_CHANGE:
			debug("WiFi mode %d -> %d\n",
				  event->event_info.auth_change.old_mode,
				  event->event_info.auth_change.new_mode);
			break;
		//Got an IP address.
		case     EVENT_STAMODE_GOT_IP:
			db_printf("WiFi got address " IPSTR ".\n",
					  IP2STR(&event->event_info.got_ip.ip));		
			//Stop the connection time out.
			os_timer_disarm(&connected_timer);
			//Reset timeout
			timeout = CONNECT_DELAY_SEC;
			wifi_connected = WIFI_MODE_CLIENT;

			/* Call the task and tell that we are now connected in
			 * station mode.
			 */
			task_raise_signal(connected_signal, wifi_connected);
			break;
		//Someone has connected to our AP.
		case     EVENT_SOFTAPMODE_STACONNECTED:
			debug("WiFi " MACSTR " connected to AP.\n", 
			          MAC2STR(event->event_info.sta_connected.mac));
			break;
		//Someone has disconnected from our AP.
		case     EVENT_SOFTAPMODE_STADISCONNECTED:
			debug("WiFi " MACSTR " disconnected from AP.\n", 
			          MAC2STR(event->event_info.sta_disconnected.mac));
			break;
		//Someone is probing our AP.
		case     EVENT_SOFTAPMODE_PROBEREQRECVED:
			debug("WiFi probe from AP (MAC: " MACSTR ").\n", 
			          MAC2STR(event->event_info.ap_probereqrecved.mac));
			break;
		default:
			warn("Unknown event %d from WiFi system.\n", event->event);
			break;
	}
}

/**
 * @brief Timer call back to check for connection timeout.
 */
static void timeout_check(void)
{
	timeout--;
	if (timeout == 0)
	{
		if (!wifi_check_connection())
		{
			db_printf("WiFi connect time out.\n");
			os_timer_disarm(&connected_timer);
			wifi_ap_mode();
			return;
		}
	}
	db_printf("WiFi connecting (%d)...\n", timeout);
}		

/**
 * @brief Connect to the configured Access Point.
 *
 */
static bool wifi_connect_default(void)
{
    unsigned char ret = 0;
    struct station_config station_conf;

	ret = wifi_get_opmode();
	if (ret == STATION_MODE)
	{
		//Get a pointer the configuration data.
		ret = wifi_station_get_config(&station_conf);
		if (!ret)
		{
			error("Cannot get station configuration (%d).", ret);
			return(false);
		}

		debug("Connecting to SSID: %s, password: %s\n", station_conf.ssid,
			  station_conf.password);

		ret = wifi_disconnect();
		if (!ret)
		{
			error("Cannot disconnect (%d).", ret);
			return(false);
		}

		//Set connections config.
		ret = wifi_station_set_config(&station_conf);
		if (!ret)
		{
			error("Failed to set station configuration (%d).\n", ret);
			return(false);
		}
		
		if (!wifi_station_connect())
		{
			error("Connect failed.\n");
			return(false);
		}
	}
	else
	{
		error("Cannot connect when not in WiFi client mode (%d).\n", ret);
		return(false);
	}
	return(true);
}

bool wifi_check_connection(void)
{
	unsigned char ret = 0;
	
	ret = wifi_get_opmode();
	if ((ret == STATION_MODE))
	{
		//We are in client mode, check for connection to an AP.
		ret = wifi_station_get_connect_status();
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

bool wifi_init(char *hostname, wifi_handler_t connected,
			   wifi_handler_t disconnected)
{
	debug("WiFi init.\n");

	//Add connection state task.
	connected_signal = task_add(connected);
	disconnected_signal = task_add(disconnected);
	//Set callback
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	
	debug("WiFi event handler %p.\n", wifi_handle_event_cb);

	if (!wifi_set_opmode(STATION_MODE))
	{
		error("Could not enter WiFi client mode.\n");
		return(false);
	}

	if (wifi_station_set_hostname(hostname))
	{
		db_printf("Host name set to %s.\n", hostname);
	}
	else
	{
		warn("Could not set host name.\n");
	}
	
	if (!wifi_connected)
	{
		if (!wifi_connect_default())
		{
			error("Could not connect to the configured AP.\n");
			return(false);
		}
		//Start connection task
		//Disarm timer
		os_timer_disarm(&connected_timer);
		//Setup timer, pass callback as parameter.
		os_timer_setfn(&connected_timer, (os_timer_func_t *)timeout_check, NULL);
		//Arm the timer, run every 1 second.
		os_timer_arm(&connected_timer, 1000, 1);
	}

	return(true);
}
