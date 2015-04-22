/* wifi_connect.c
 *
 * Routines for connecting th ESP8266 to a WIFI network.
 *
 * - Load a WIFI configuration.
 * - Try to connect to the AP.
 * - If unsuccessful create an AP presenting a HTTP to configure the 
 *   connection values.
 * 
 * What works.
 * - Nothin'.
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

//AP list.
struct bss_info *bss_list = NULL;

//Internal function to set station mode.
static unsigned char ICACHE_FLASH_ATTR set_opmode(unsigned char mode)
{
    debug("Setting WIFI mode: ");
    
#ifdef DEBUG
    switch(mode)
    {
        case NULL_MODE:         
            debug("NULL mode.");
            break;
                        
        case STATION_MODE:      
            debug("Station mode.");
            break;

        case SOFTAP_MODE:
            debug("Soft AP mode.");
            break;

        case STATIONAP_MODE:    
            debug("Station and soft AP mode.");
            break;
                        
        default:                
            debug("Unknown mode.");
            break;
    }
    debug("\n");
#endif
    return(wifi_set_opmode(mode));
}

/* Connect to an Access Point.
 * 
 * Parameters:
 *  - char *ssid: Pointer to the SSID for the AP.
 *  - char *passwd: Pointer to the password for the AP.
 * Returns:
 *  Connection status from SDK API call to wifi_station_get_connect_status.
 *   enum{
 *      STATION_IDLE = 0,
 *      STATION_CONNECTING,
 *      STATION_WRONG_PASSWORD,
 *      STATION_NO_AP_FOUND,
 *      STATION_CONNECT_FAIL,
 *      STATION_GOT_IP
 *   };
 */
unsigned char ICACHE_FLASH_ATTR connect_ap(char *ssid, char *passwd)
{
    struct station_config station_conf = {{0}};
    unsigned char   connect_status = 0;
    unsigned char   i = 0;

    //Set station mode
    set_opmode(STATION_MODE);

    //Get a pointer the configuration data.
    if (!wifi_station_get_config(&station_conf))
    {
        os_printf("Can not get station configuration.");
        return(8);
    }
    
    os_memset(station_conf.ssid, 0, sizeof(station_conf.ssid));
	os_memset(station_conf.password, 0, sizeof(station_conf.password));
    //Set ap settings
    os_strcpy((char *)station_conf.ssid, ssid);
    os_strcpy((char *)station_conf.password, passwd);
    //No BSSID
    station_conf.bssid_set = 0;

    debug("Connecting to SSID: %s, password: %s\n", station_conf.ssid, station_conf.password);
    debug("Retries %d\n", CONNECT_DELAY_SEC);

	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
    
    //Connect to AP.
//    ETS_UART_INTR_DISABLE();
    if (!wifi_station_set_config(&station_conf))
    {
//        ETS_UART_INTR_ENABLE();
        os_printf("Failed to set station configuration.\n");
        return(9);
    }
    //ETS_UART_INTR_ENABLE();
    
    //Connect to AP if necessary.
    connect_status = wifi_station_get_connect_status();
    if ((connect_status != STATION_GOT_IP) ||
        (connect_status != STATION_CONNECTING))
    {
        if (!wifi_station_connect())
        {
            os_printf("Failed to connect to AP.\n");
            return(10);
        }
    }

    //Wait a while for a connection.
    while((connect_status != STATION_GOT_IP) && (i < CONNECT_DELAY_SEC))
    {
        os_delay_us(1000000);
        connect_status = wifi_station_get_connect_status();
        debug("Connection status: %d, attempt %d\n", connect_status, i);
        i++;
    }
    
    return(connect_status);
}


//Internal callback for scanning for AP's.
static void ICACHE_FLASH_ATTR scan_done(void *arg, STATUS status)
{
    if (status == OK)
    {
        bss_list = (struct bss_info *)arg;
        debug("AP scan completed successfully.\n");
    }
    debug("AP scan failed.\n");
} 

/* Scan for Access Points and return a list of available ones..
 * 
 * Parameters:
 *  - char *ssid: Pointer to the SSID for the AP.
 *  - char *passwd: Pointer to the password for the AP.
 * Returns:
 *  0 = fail, 1 = success.
 */
unsigned char ICACHE_FLASH_ATTR scan_ap(void)
{
    unsigned int    i = 0;
    unsigned char   ret;
    
    debug("Scanning for AP's.\n");
    wifi_station_disconnect();
    
    ret = wifi_station_scan(NULL, scan_done);
    if (ret)
    {
        //Wait a for the scan_done callback.
        while((bss_list == NULL) && (i < CONNECT_DELAY_SEC))
        {
            os_delay_us(1000000);
            debug("Waiting for AP list...\n");
            i++;
        }
        return(1);
    }
    debug("AP scan failed.\n");
    return(0);
}   

/* Print list of Access Points.
 * Parameters:
 *  none
 * Return:
 * none
 */
void ICACHE_FLASH_ATTR print_ap_list(void)
{
    struct bss_info *bss = bss_list;
    
    os_printf("Access point list:\n");
    while(bss != NULL)
    {
        os_printf("%s\n", bss->ssid);
        bss = bss->next.stqe_next;
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
    
    debug("Creating AP SSID: %s, password: %s, channel %u\n", ssid, passwd, 
          channel);
          
    //Set AP mode.
    set_opmode(SOFTAP_MODE);
              
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
    wifi_softap_set_config(&ap_config);
}

void ICACHE_FLASH_ATTR no_ap(void)
{
    unsigned char           mac[6];
    char                    ssid[32];
    char                    passwd[10];
    unsigned char           i;
    char                    temp_str[3];
    
    os_printf("No AP connection. Entering AP configuration mode.\n");
    
    //Get MAC address.
    wifi_get_macaddr(SOFTAP_IF, mac);
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
    strcpy(passwd, "1234567890");

    //Create the AP.
    create_softap(ssid, passwd, 6);
}

