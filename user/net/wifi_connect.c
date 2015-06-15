/**
 * @file wifi_connect.c
 *
 * @brief Routines for connecting th ESP8266 to a WIFI network.
 *
 * - Load a WIFI configuration.
 * - If unsuccessful create an AP.
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
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "tools/strxtra.h"
#include "wifi_connect.h"

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
 * @brief Retry counter.
 */
unsigned char   retries = CONNECT_DELAY_SEC * (1000 / DISPATCH_TIME);

/**
 * @brief Disconnect from an access point.
 *
 * @return `true` on success.
 */
static bool ICACHE_FLASH_ATTR wifi_disconnect(void)
{
    unsigned char   ret;
    
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
 * @brief Print the status, from the SDK numeric code.
 * 
 * @param status The status code to print.
 */
static void ICACHE_FLASH_ATTR print_status(unsigned char status)
{
    switch(status)
    {
        case STATION_IDLE:
            db_printf("Idle...\n");
            break;
        case STATION_CONNECTING:
            db_printf("Connecting...\n");
            break;
        case STATION_WRONG_PASSWORD:
            db_printf("Wrong password...\n");
            break;            
        case STATION_NO_AP_FOUND:
            db_printf("Could not find an AP...\n");
            break;
        case STATION_CONNECT_FAIL:
            db_printf("Connection failed...\n");
            break;
        case STATION_GOT_IP:
            db_printf("Connected.\n");
            break;
        default:
            db_printf("Unknown status: %d\n", status);
            break;    
    }
}  

/**
 * @brief Check if we're connected.
 * 
 * @param connected Pointer to a boolean telling whether we're connected.
 * @return `true`on connection or time out. False otherwise.
 */
bool ICACHE_FLASH_ATTR check_connect(bool *connected)
{
    struct ip_info  ipinfo;
    unsigned char   ret;
    unsigned char connect_status;
 
    retries--;
    if (retries == 0)
    {
		return(true);
    }   
    //Get connection status
    connect_status = wifi_station_get_connect_status();
    switch(connect_status)
    {
        case STATION_GOT_IP:
            print_status(connect_status);
            //Print IP address
            ret = wifi_get_ip_info(STATION_IF, &ipinfo);
            if (!ret)
            {
                error("Failed get IP address (%d).\n", ret);
                return(false);
            }
            db_printf("Got IP address: %d.%d.%d.%d.\n", IP2STR(&ipinfo.ip) );
            *connected = true;
            return(true);
            break;
        //Not connected
        case STATION_IDLE: 
        case STATION_WRONG_PASSWORD:
        case STATION_NO_AP_FOUND:
        case STATION_CONNECT_FAIL:
            print_status(connect_status);
            //Connect
            if (wifi_station_connect())
            {
                db_printf("Retrying...\n");
            }
            else
            {
                error("Failed to connect\n");
            }
            *connected = false;
            return(false);
            break;
        default:
			*connected = false;
            return(false);
            break;    
    }
}

/**
 * @brief Create an Access Point.
 * 
 * @param ssid The SSID for the AP.
 * @param passwd The password for the AP.
 * @param channel Channel for the AP.
 */
static bool ICACHE_FLASH_ATTR create_softap(char *ssid, char *passwd, unsigned char channel)
{
    struct softap_config    ap_config;
    unsigned char           ret;
    
    debug("Creating AP SSID: %s, password: %s, channel %u\n", ssid, passwd, 
          channel);
          
    //Set AP mode.
    ret = wifi_set_opmode_current(STATIONAP_MODE);
    if (!ret)
    {
        error("Cannot set soft AP mode (%d).", ret);
        return(false);
    }
              
    //Populate the struct, needed to configure the AP.
    os_strcpy((char *)ap_config.ssid, ssid);
    os_strcpy((char *)ap_config.password, passwd);
    ap_config.ssid_len = os_strlen((char *)ap_config.ssid);
    ap_config.channel = channel;
    //WPA & WPA2 Pre Shared Key (e.g "home" networks).
    ap_config.authmode = AUTH_WPA_WPA2_PSK;
    ap_config.ssid_hidden = 0;
    //We don't need a lot of connections for our purposes.
    ap_config.max_connection = 2;
    ap_config.beacon_interval = 100;
    
    //Set AP configuration.
    ret = wifi_softap_set_config(&ap_config);
    if (!ret)
    {
        error("Failed to set soft AP configuration (%d).", ret);
        return(false);
    }
    return(true);
}

/**
 * @brief Create a default access point.
 * 
 * @param connected Pointer to a boolean, that will be set to true on success.
 * @return `true` on success.
 */
bool ICACHE_FLASH_ATTR setup_ap(bool *connected)
{
    unsigned char           mac[6];
    char                    ssid[16];
    unsigned char           i;
    char                    temp_str[3];
    unsigned char           ret;
 
    debug("WIFI connection time out.\n");   
    db_printf("No AP connection. Entering AP configuration mode.\n");
    
    //Get MAC address.
    ret = wifi_get_macaddr(SOFTAP_IF, mac);
    if (!ret)
    {
        error("Cannot get MAC address (%d).", ret);
        return(false);
    }
    debug("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", MAC2STR(mac));
    
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
    if (create_softap(ssid, SOFTAP_PASSWORD, 6))
    {
		*connected = true;
		return(true);
	}
	return(false);
}

/**
 * @brief Start connecting to the default access point.
 * 
 * @param connected Pointer to a boolean telling whether we're connected.
 * @param `true` on success (which just means we're connecting).
 */
bool ICACHE_FLASH_ATTR setup_station(bool *connected)
{
    unsigned char ret = 0;

    //Set station mode
    ret = wifi_set_opmode(STATION_MODE);
    if (!ret)
    {
        error("Cannot set station mode (%d).", ret);
        return(false);
    }
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
    *connected = false;
    
    //Set connections config.
    ret = wifi_station_set_config(&station_conf);
    if (!ret)
    {
        error("Failed to set station configuration (%d).\n", ret);
        return(false);
    }
    return(true);
}
