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
    wifi_set_opmode( SOFTAP_MODE );
              
    //Populate the struct, needed to configure the AP.
    strcpy(ap_config.ssid, ssid);
    strcpy(ap_config.password, passwd);
    ap_config.ssid_len = strlen(ap_config.ssid);
    ap_config.channel = channel;
    //WPA & WPA2 Pre Shared Key (e.g "home" networks).
    ap_config.authmode = AUTH_WPA_WPA2_PSK;
    ap_config.ssid_hidden = 0;
    //We don't need a lot of connections for out purposes.
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

