/**
 * @file wifi.c
 *
 * @brief Routines for connecting th ESP8266 to a WIFI network.
 *
 * - Load a WIFI configuration.
 * - If unsuccessful create an AP presenting a HTTP server to configure 
 *   the connection values.
 * 
 * What works:
 * - Try to connect to the AP.
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
#include "os_type.h"
#include "user_interface.h"
#include "osapi.h"
#include "user_config.h"
#include "tools/missing_dec.h"
#include "tools/strxtra.h"
#include "wifi.h"

/**
 * @brief The current station config.
 * 
 * Configuration used, when connecting to an AP.
 */
struct station_config station_conf = {{0}};
/**
 * @brief The current AP configuration.
 * 
 * Configuration used when in connection config mode.
 */
struct softap_config  ap_config = {{0}};

/**
 * @brief Connection mode.
 * 
 * Values:
 *  0 No connection.
 *  1 Normal.
 *  2 Config.
 */
static unsigned char wifi_connected = WIFI_MODE_NO_CONNECTION;

/**
 * @brief Connection callback function.
 *
 * @param wifi_mode WiFi mode: 1 = Connected to AP, 2 = Config mode.
 */
static void (*wifi_connected_cb)(unsigned char wifi_mode);
/**
 * @brief Disonnect callback function.
 *
 */
static void (*wifi_disconnected_cb)(void);
//Retry counter.
static unsigned char timeout = CONNECT_DELAY_SEC;
//Timer for waiting for connection.
static os_timer_t connected_timer;

//Forward declarations.
static bool wifi_connect_default(void);

/**
 * @brief Disconnect WIFI.
 * 
 * @return `true`on success.
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
            error("Cannot disconnect (%d).", ret);
            return(false);
        }
        ret = wifi_station_dhcpc_stop();
        if (!ret)
        {
            error("Cannot stop DHCP client (%d).", ret);
            return(false);
        }
    }
    return(true);
}

/**
 * @brief Switch to Access Point mode.
 * 
 * Swithches the ESP8266 to STATIONAP mode, where both AP, and station mode is active.
 *
 * @param ssid Pointer to the SSID for the AP.
 * @param passwd Pointer to the password for the AP.
 * @param channel Channel for the AP.
 * @return true on success, false on failurre.
 */
static bool create_softap(char *ssid, char *passwd, unsigned char channel)
{
    unsigned char           ret;

    debug("Creating AP SSID: %s, password: %s, channel %u\n", ssid, passwd, 
          channel);

    //Set station + AP mode. Station mode is needed for scanning available networks.
    if (!wifi_set_opmode(STATIONAP_MODE))
    {
        error("Cannot set soft AP mode.\n");
        return(false);
    }
    if (!wifi_softap_get_config(&ap_config))
    {
        error("Cannot set get default AP mode configuration.\n");
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
    ap_config.max_connection = 4;
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
static bool wifi_config_mode(void)
{
    unsigned char           mac[6];
    char                    ssid[32];
    char                    passwd[11];
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

    //The password.
    strcpy(passwd, SOFTAP_PASSWORD);

    //Create the AP.
    if (!create_softap(ssid, passwd, 10))
    {
		error("Could not start configuration mode.\n");
		return(false);
	}
    
    wifi_connected = WIFI_MODE_CONFIG;
    //Reset retry counter.
    timeout = CONNECT_DELAY_SEC;
    //Call connected call back.
    wifi_connected_cb(wifi_connected);

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
			debug("Disconnected from AP.\n");
			debug("Reason: %d.\n", event->event_info.disconnected.reason);
			//Ignore failed client attempts in config mode.
			if (wifi_connected < WIFI_MODE_CONFIG)
			{
				debug("Disconnected from Wifi AP.\n");
				wifi_connected = WIFI_MODE_NO_CONNECTION;
				if (wifi_disconnected_cb)
				{
					wifi_disconnected_cb();
				}
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
			//We do not want to be connected in config mode.
			if (wifi_get_opmode() > STATION_MODE)
			{
				wifi_set_opmode(STATION_MODE);
				if (!wifi_connect_default())
				{
					error("Could not connect to the configured AP.\n");
					return;
				}
			}
				
			//Reset timeout
			timeout = CONNECT_DELAY_SEC;
			wifi_connected = WIFI_MODE_CLIENT;
			//Stop the connection time out.
			os_timer_disarm(&connected_timer);
			/* Call the call back and tell that we are now connected in
			 * station mode.
			 */
			if (wifi_connected_cb)
			{
				wifi_connected_cb(STATION_MODE);
			}
			break;
		//Someone has connected to our configuration AP.
		case     EVENT_SOFTAPMODE_STACONNECTED:
			debug("WiFi " MACSTR " connected to AP.\n", 
			          MAC2STR(event->event_info.sta_connected.mac));
			break;
		//Someone has disconnected to out configuration AP.
		case     EVENT_SOFTAPMODE_STADISCONNECTED:
			debug("WiFi " MACSTR " disconnected from AP.\n", 
			          MAC2STR(event->event_info.sta_disconnected.mac));
			break;
				//Someone has disconnected to out configuration AP.
		case     EVENT_SOFTAPMODE_PROBEREQRECVED:
			debug("WiFi " MACSTR " probe from AP.\n", 
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
			//Connected call off everything and reset it.
			os_timer_disarm(&connected_timer);
			wifi_config_mode();
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
    unsigned char   ret = 0;

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

/**
 * @brief Check if we're connected to a WIFI.
 *
 * Both station mode with an IP address and AP mode count as connected.
 * 
 * @return `true`on connection.
 */
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

/**
 * @brief Intitialise WIFI connection, by waiting for auto
 * connection to succed, manually connecting to the default
 * configured AP, or creating an AP named ESP+$mac.
 * 
 * @param hostname The desired host name used by the DHCP client.
 * @param connect_cb Function to call when a connection is made.
 * @param disconnect_cb Function to call when a disconnect happens.
 */
bool wifi_init(char *hostname, 
								 void (*connect_cb)(unsigned char wifi_mode),
								 void (*disconnect_cb)())
{
	debug("WiFi init.\n");
	//Set callback
    wifi_connected_cb = connect_cb;
    wifi_disconnected_cb = disconnect_cb;
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	
	debug("WiFi call backs connected %p, disconnected %p, and event handler %p.\n", 
		  wifi_connected_cb, wifi_disconnected_cb,
		  wifi_handle_event_cb);

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
