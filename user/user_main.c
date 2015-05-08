/**
 * 
 * @file user_main.c
 * 
 * @brief Main program file.
 * 
 * @copyright
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
 * @license
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
/**
 * @mainpage WIFISwitch source docs.
 * 
 * Introduction.
 * =============
 * 
 * This is lalkalkjnhgjs.
 * 
 */
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "missing_dec.h"
#include "wifi_connect.h"
#include "tcp.h"
#include "http.h"

/**
 * @brief Number of built in URIs in the #g_builtin_uris array.
 */
#define N_BUILTIN_URIS  2

char *hello(char *uri);
char *idx(char *uri);

/**
 * @brief Array of built in handlers and their URIs.
 */
struct http_builtin_uri g_builtin_uris[N_BUILTIN_URIS] =
{
    {"/", idx},
    {"/hello", hello}
};

char *hello(char *uri)
{
    char    *html = "<!DOCTYPE html><head><title>Web server test.</title></head>\
                     <body>Hello world.</body></html>";
    return(html);
}

char *idx(char *uri)
{
    char    *html = "<!DOCTYPE html><head><title>Index.</title></head>\
                     <body>It works.<br /><form action=\"/post\" method=\"POST\"><br /><input type=\"submit\" \
                     value=\"POST\"></form> </body></html>";
    return(html);
}

/**
 * @brief This functions is called when a WIFI connection has been established.
 */
void ICACHE_FLASH_ATTR connected_cb(void)
{
    init_http(g_builtin_uris, N_BUILTIN_URIS);
}

/**
 * @brief Main entry point and init code.
 */
void ICACHE_FLASH_ATTR user_init(void)
{
    //Turn off auto connect.
    wifi_station_set_auto_connect(false);
    
    // Set baud rate of debug port
    uart_div_modify(0,UART_CLK_FREQ / 115200);
    
    os_delay_us(1000);
    
    //Print banner
    os_printf("\nWIFISwitch version %s.\n", STRING_VERSION);
    system_print_meminfo();
    os_printf("Free heap %u\n\n", system_get_free_heap_size());    
           
    //flash_dump(0x07600, 65536);
    
    wifi_connect(connected_cb);
    
    os_printf("\nLeaving user_init...\n");
}
