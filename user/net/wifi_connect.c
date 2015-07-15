/**
 * @file wifi_connect.c
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
 * @brief Connection callback function.
 *
 * @param wifi_mode 1 = client mode. 2 = AP mode. 3 = client + AP mode.
 */
void (*wifi_connected_cb)(unsigned char wifi_mode);
//Internal connection timeout callback function.
void (*int_timeout_cb)(void);
//Timer for waiting for connection.
static os_timer_t connected_timer;
//Retry counter.
unsigned char   retries = CONNECT_DELAY_SEC;

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
 * @brief Print the status, from the numeric one
 *
 * @param status Status of the WIFI connaction as returned from the ESP8266 SDK.
 */
static void ICACHE_FLASH_ATTR print_status(unsigned char status)
{
    switch(status)
    {
		db_printf("WIFI connection ");
        case STATION_IDLE:
            db_printf("is idle.\n");
            break;
        case STATION_CONNECTING:
            db_printf("is connecting.\n");
            break;
        case STATION_WRONG_PASSWORD:
            db_printf("has wrong password.\n");
            break;
        case STATION_NO_AP_FOUND:
            db_printf("could not find an AP.\n");
            break;
        case STATION_CONNECT_FAIL:
            db_printf("has failed.\n");
            break;
        case STATION_GOT_IP:
            db_printf("is connected.\n");
            break;
        default:
            db_printf("has unknown status: %d\n", status);
            break;
    }
}  

/**
 * @brief Connect to WIFI network.
 */
static void connect(void)
{
    struct ip_info ipinfo;
    unsigned char ret;
    unsigned char connect_status;

    //Get connection status
    connect_status = wifi_station_get_connect_status();
    switch(connect_status)
    {
        case STATION_GOT_IP:
            print_status(connect_status);
            //Stop calling me
            os_timer_disarm(&connected_timer);
            //Print IP address
            ret = wifi_get_ip_info(STATION_IF, &ipinfo);
            if (!ret)
            {
                error("Failed to get IP address (%d).\n", ret);
                return;
            }
            db_printf("Got IP address: %d.%d.%d.%d.\n", IP2STR(&ipinfo.ip) );
            //Call callback.
            wifi_connected_cb(1);
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
                db_printf("Retrying WIFI connection...\n");
            }
            else
            {
                error("Failed to connect.\n");
            }
            break;
        default:
            print_status(connect_status);
            break;
    }
    retries--;
    if (retries == 0)
    {
        //Reset retries.
        retries = CONNECT_DELAY_SEC;

        //Stop calling me.
        os_timer_disarm(&connected_timer);

        wifi_disconnect();
        //Call the timeout callback.
        int_timeout_cb();
    }
}

/**
 * @brief Switch to Access Point mode.
 * 
 * Swithches the ESP8266 to STATIONAP mode, where both AP, and station mode is active.
 *
 * @param ssid Pointer to the SSID for the AP.
 * @param passwd Pointer to the password for the AP.
 * @param channel Channel for the AP.
 */
static void ICACHE_FLASH_ATTR create_softap(char *ssid, char *passwd, unsigned char channel)
{
    unsigned char           ret;

    debug("Creating AP SSID: %s, password: %s, channel %u\n", ssid, passwd, 
          channel);

    //Set station + AP mode. Station mode is needed for scanning available networks.
    ret = wifi_set_opmode(STATIONAP_MODE);
    if (!ret)
    {
        error("Cannot set soft AP mode (%d).", ret);
        return;
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
        return;
    }
}

/**
 * @brief This function is called when a WIFI connection attempt has timed out.
 *
 * Creates an access point named ESP+MAC_ADDR with the password of #SOFTAP_PASSWORD.
 */
void timeout_cb(void)
{
    unsigned char           mac[6];
    char                    ssid[32];
    char                    passwd[10];
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
        return;
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

    //The password.
    strcpy(passwd, SOFTAP_PASSWORD);

    //Create the AP.
    create_softap(ssid, passwd, 6);
    
    //Call connected call back.
    wifi_connected_cb(3);
}

/*
 * @brief Connect to the configured Access Point.
 *
 * @param connect_cb Called when a connection has been made, or an AP has been created.
 */
void ICACHE_FLASH_ATTR wifi_connect(void (*connect_cb)(unsigned char wifi_mode))
{
    unsigned char   ret = 0;

    //Set callback
    wifi_connected_cb = connect_cb;
    int_timeout_cb = timeout_cb;

    //Set station mode
    ret = wifi_set_opmode(STATION_MODE);
    if (!ret)
    {
        error("Cannot set station mode (%d).", ret);
        return;
    }
    //Get a pointer the configuration data.
    ret = wifi_station_get_config(&station_conf);
    if (!ret)
    {
        error("Cannot get station configuration (%d).", ret);
        return;
    }

    debug("Connecting to SSID: %s, password: %s\n", station_conf.ssid,
          station_conf.password);

    ret = wifi_disconnect();
    if (!ret)
    {
        error("Cannot disconnect (%d).", ret);
        return;
    }

    //Set connections config.
    ret = wifi_station_set_config(&station_conf);
    if (!ret)
    {
        error("Failed to set station configuration (%d).\n", ret);
        return;
    }

    //Start connection task
    //Disarm timer
    os_timer_disarm(&connected_timer);
    //Setup timer, pass callback as parameter.
    os_timer_setfn(&connected_timer, (os_timer_func_t *)connect, NULL);
    //Arm the timer, run every 1 second.
    os_timer_arm(&connected_timer, 1000, 1);
}


/**
 * @brief Check if we're connected to a WIFI.
 *
 * Both station mode with an IP address and AP mode count as connected.
 * 
 * @return `true`on connection.
 */
bool ICACHE_FLASH_ATTR wifi_check_connection(void)
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
