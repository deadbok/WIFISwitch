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
 * Source level documentation for a network controlled mains switch, using the
 * ESP8266 WIFI interface and micro controller.
 * 
 * @tableofcontents
 * 
 */
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "tools/missing_dec.h"
#include "net/wifi_connect.h"
#include "net/tcp.h"
#include "slighttp/http.h"
#include "led.h"
#include "int_flash.h"
#include "fs/fs.h"

/**
 * @brief Number of built in URIs in the #g_builtin_uris array.
 */
#define N_BUILTIN_URIS  3

char *hello(char *uri, struct http_request *request, struct http_response *response);
bool hello_test(char *uri);

char *idx(char *uri, struct http_request *request, struct http_response *response);
bool idx_test(char *uri);

/**
 * @brief Array of built in handlers and their URIs.
 */
struct http_builtin_uri g_builtin_uris[N_BUILTIN_URIS] =
{
    {idx_test, idx, NULL},
    {hello_test, hello, NULL},
    {led_test, led_html, NULL}
};

bool ICACHE_FLASH_ATTR hello_test(char *uri)
{
    if (os_strcmp(uri, "/hello") == 0)
    {
        return(true);
    }
    return(false);
}

char ICACHE_FLASH_ATTR *hello(char *uri, struct http_request *request, struct http_response *response)
{
    char    *html = "<!DOCTYPE html><head><title>Web server test.</title></head>\
                     <body>Hello world.</body></html>";
    return(html);
}

bool ICACHE_FLASH_ATTR idx_test(char *uri)
{
    if (os_strlen(uri) == 1)
    {
        return (true);
    }
    if (os_strncmp(uri, "/?", 2) == 0)
    {
        return(true);
    }
    if (os_strncmp(uri, "/index.html", 11) == 0)
    {
        return(true);
    }
    return(false);  
}

char ICACHE_FLASH_ATTR *idx(char *uri, struct http_request *request, struct http_response *response)
{
    unsigned char i;
    unsigned char offset = 0;
    char    *data = NULL            ;
    char    *html = "<!DOCTYPE html><head><title>Index.</title></head>\
                     <body>It works.<br /><form action=\"/\" method=\"POST\">\
                     <input type=\"text\" name=\"posttext\" value=\"testpost\">\
                     <br /><input type=\"submit\" value=\"POST\">\
                     </form><br /><form action=\"/\" method=\"GET\">\
                     <input type=\"text\" name=\"gettext\" value=\"testget\">\
                     <br /><input type=\"submit\" value=\"GET\"></form>\
                     <div name=\"result\">               </div></body></html>";
    char    *html_tmpl;
    
    //Find the replaceable part of the HTML.
    html_tmpl = os_strstr(html, "<div name=\"result\">");
    if (html_tmpl)
    {
        html_tmpl += 19;
    }
    //Handle GET request.
    if (request->type == HTTP_GET)
    {
        //Find the data in the URI.
        data = os_strstr(uri, "gettext=");
        offset = 8;
        *html_tmpl++ = ' ';
        *html_tmpl++ = 'G';
        *html_tmpl++ = 'e';
        *html_tmpl++ = 't';
        *html_tmpl++ = ':';        
    }
    else if (request->type == HTTP_POST)
    {
        data = os_strstr(request->message, "posttext=");
        offset = 9;
        *html_tmpl++ = 'P';
        *html_tmpl++ = 'o';
        *html_tmpl++ = 's';
        *html_tmpl++ = 't';
        *html_tmpl++ = ':';
    }
    if (data && html_tmpl)
    {
        *html_tmpl++ = ' ';
        data += offset;

        for (i = 0; i < 8; i++)
        {
            if (*data == '\0')
            {
                *html_tmpl++ = ' ';
            }
            else
            {
                *html_tmpl++ = *data++;
            }
        }
    }
    return(html);
}

/**
 * @brief This functions is called when a WIFI connection has been established.
 */
void ICACHE_FLASH_ATTR connected_cb(void)
{
    init_http("/", g_builtin_uris, N_BUILTIN_URIS);
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
    debug("IRAM: %p to %p.\n", _irom0_text_start, _irom0_text_end);
           
    //flash_dump_mem(0x14000, 512);
    
    fs_init();
    
    wifi_connect(connected_cb);
    
    // Initialize the GPIO subsystem.
    gpio_init();

    //Set GPIO2 to output mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

    //Set GPIO2 low
    gpio_output_set(0, BIT5, BIT5, 0);
    
    os_printf("\nLeaving user_init...\n");
}
