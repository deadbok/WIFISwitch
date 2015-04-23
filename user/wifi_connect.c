/* wifi_connect.c
 *
 * Routines for connecting th ESP8266 to a WIFI network.
 *
 * - Load a WIFI configuration.
 * - If unsuccessful create an AP presenting a HTTP to configure the 
 *   connection values.
 * 
 * What works.
 * - Try to connect to the AP.
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
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "itoa.h"
#include "missing_dec.h"
#include "wifi_connect.h"

//The current station config.
struct station_config station_conf = {{0}};
//The current AP configuration
struct softap_config  ap_config = {{0}};
//Connection status
unsigned char         connect_status = 0;

//Connection callback function.
void (*wifi_connected_cb)(void);
//Internal connection timeout callback function.
void (*int_timeout_cb)(void);
//Timer for waiting for connection.
static os_timer_t connected_timer;
//Retry counter.
unsigned char   retries = CONNECT_DELAY_SEC;

//Internal function to disconnect from an AP
static unsigned char ICACHE_FLASH_ATTR wifi_disconnect(void)
{
    unsigned char   ret;
    
    //Disconnect if connected.
    connect_status = wifi_station_get_connect_status();
    if (connect_status == STATION_GOT_IP)
    {
        debug("Disconnecting.\n");
        ret = wifi_station_disconnect();
        if (!ret)
        {
            os_printf("Cannot disconnect (%d).", ret);
            return(ret);
        }
        ret = wifi_station_dhcpc_stop();
        if (!ret)
        {
            os_printf("Cannot stop DHCP client (%d).", ret);
            return(255);
        }    
    }
    return(1);
}

//Print the status, from the numeric one
static void ICACHE_FLASH_ATTR print_status(unsigned char status)
{
    switch(status)
    {
        case STATION_IDLE:
            os_printf("Idle...\n");
            break;
        case STATION_CONNECTING:
            os_printf("Connecting...\n");
            break;
        case STATION_WRONG_PASSWORD:
            os_printf("Wrong password...\n");
            break;            
        case STATION_NO_AP_FOUND:
            os_printf("Could not find an AP...\n");
            break;
        case STATION_CONNECT_FAIL:
            os_printf("Idle...\n");
            break;
        case STATION_GOT_IP:
            os_printf("Connected.\n");
            break;
        default:
            os_printf("Unknown status: %d\n", status);
            break;    
    }
}  

//Internal task to connect to an AP
static void connect(void *arg)
{
    struct ip_info  ipinfo;
    unsigned char   ret;
    
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
                os_printf("Failed get IP address (%d).\n", ret);
                return;
            }
            os_printf("Got IP address: %d.%d.%d.%d.\n", IP2STR(&ipinfo.ip) );
            //Call callback.
            wifi_connected_cb();
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
                os_printf("Trying to connect...\n");
            }
            else
            {
                os_printf("Failed to connect\n");
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
        
        //Call the timeout callback.
        int_timeout_cb();
    }
}

/* Create an Access Point.
 * 
 * Parameters:
 *  - char *ssid: Pointer to the SSID for the AP.
 *  - char *passwd: Pointer to the password for the AP.
 *  - unsigned char channel: Channel for the AP.
 */
static void ICACHE_FLASH_ATTR create_softap(char *ssid, char *passwd, unsigned char channel)
{
    struct softap_config    ap_config;
    unsigned char           ret;
    
    debug("Creating AP SSID: %s, password: %s, channel %u\n", ssid, passwd, 
          channel);
          
    //Set AP mode.
    ret = wifi_set_opmode(SOFTAP_MODE);
    if (!ret)
    {
        os_printf("Cannot set soft AP mode (%d).", ret);
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
        os_printf("Failed to set soft AP configuration (%d).", ret);
        return;
    }
}

//This functions is called when a WIFI connection attempt has timed out.
void timeout_cb(void)
{
    unsigned char           mac[6];
    char                    ssid[32];
    char                    passwd[10];
    unsigned char           i;
    char                    temp_str[3];
    unsigned char           ret;
 
    debug("WIFI connection time out.\n");   
    os_printf("No AP connection. Entering AP configuration mode.\n");
    
    //Get MAC address.
    ret = wifi_get_macaddr(SOFTAP_IF, mac);
    if (!ret)
    {
        os_printf("Cannot get MAC address (%d).", ret);
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
}

/* Connect to the configured Access Point.
 * 
 * Parameters:
 *  none.
 * Returns:
 *  none.
 */
void ICACHE_FLASH_ATTR wifi_connect(void (*connect_cb)())
{
    unsigned char   ret = 0;

    //Set callback
    wifi_connected_cb = connect_cb;
    int_timeout_cb = timeout_cb;
    
    debug("Connecting to SSID: %s, password: %s\n", station_conf.ssid, station_conf.password);
    //Set station mode
    ret = wifi_set_opmode(STATION_MODE);
    if (!ret)
    {
        os_printf("Cannot set station mode (%d).", ret);
        return;
    }
    //Get a pointer the configuration data.
    ret = wifi_station_get_config(&station_conf); 
    if (!ret)
    {
        os_printf("Cannot get station configuration (%d).", ret);
        return;
    }

    ret = wifi_disconnect();
    if (!ret)
    {
        os_printf("Cannot disconnect (%d).", ret);
        return;
    }
    
    //Connect to AP.
    ret = wifi_station_set_config(&station_conf);
    if (!ret)
    {
        os_printf("Failed to set station configuration (%d).\n", ret);
        return;
    }
    
    //Start connection task
    //Disarm timer
    os_timer_disarm(&connected_timer);
    //Setup timer, pass callbaack as parameter.
    os_timer_setfn(&connected_timer, (os_timer_func_t *)connect, NULL);
    //Arm the timer, run every 1 second.
    os_timer_arm(&connected_timer, 1000, 1);
}
