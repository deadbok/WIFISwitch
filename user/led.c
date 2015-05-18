/**
 * @file led.c
 *
 * @brief Routines for blinking led with web control.
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
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "gpio.h"
#include "slighttp/http.h"

bool ICACHE_FLASH_ATTR led_test(char *uri)
{
    if (os_strncmp(uri, "/led", 4) == 0)
    {
        return(true);
    }
    return(false);  
}

char ICACHE_FLASH_ATTR *led_html(char *uri, struct http_request *request)
{
    char    *html = "<!DOCTYPE html><head><title>Led toggle.</title></head>\
                     <body>Toggle LED.<br /><div name=\"status\">LED status: Off\
                     </div><br /><form action=\"/led\" method=\"GET\">\
                     <input type=\"submit\" value=\"Toggle\"></form>\
                     </body></html>";
    char *html_tmpl;
    unsigned char led_state;
    
    //Find the replaceable part of the HTML.
    html_tmpl = os_strstr(html, "status: ");
    if (html_tmpl)
    {
        led_state = GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT5;
        
        html_tmpl += 8;
        
        if (led_state)
        {
            //Set GPIO2 to LOW
            gpio_output_set(0, BIT5, BIT5, 0);

            *html_tmpl++ = 'O';
            *html_tmpl++ = 'f';
            *html_tmpl++ = 'f';
        }
        else
        {
            //Set GPIO2 to HIGH
            gpio_output_set(BIT5, 0, BIT5, 0);

            *html_tmpl++ = 'O';
            *html_tmpl++ = 'n';
            *html_tmpl++ = ' ';
        }
    }
    return(html);
}
