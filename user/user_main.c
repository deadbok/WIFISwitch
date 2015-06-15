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
#include "fs/fs.h"
#include "sm.h"

/**
 * @brief Timer for handling events.
 */
static os_timer_t dispatch_timer;

/**
 * @brief Called by the timer to handle events.
 */
void handle_events(void)
{
	static state_t state = WIFI_CONNECT;
	static bool go_away = false;
	static struct sm_context context;
#ifdef DEBUG
	static state_t last_state = WIFI_CONNECT;
#endif
	
	if (go_away)
	{
		warn("Oops already here.\n");
	}
	
	go_away = true;
#ifdef DEBUG
	last_state = state;
#endif
	if (handlers[state])
	{
		state = (*handlers[state])(&context);
	}
	else
	{
		warn("Empty handler for state %d.\n", state);
	}

#ifdef DEBUG
	if (last_state != state)
	{
		db_printf("New state: %d:\n", state);
	}
#endif

	if (state >= N_STATES)
	{
		error("Main state machine state overflow.");
		state = 0;
	}

	go_away = false;
}

/**
 * @brief Called when initialisation is over.
 * 
 * Starts the timer that will handle events.
 */
void start_handler(void)
{
	db_printf("Running...\n");
    //Start handler task
    //Disarm timer
    os_timer_disarm(&dispatch_timer);
    //Setup timer, pass callback as parameter.
    os_timer_setfn(&dispatch_timer, (os_timer_func_t *)handle_events, NULL);
    //Arm the timer, run every DISPATCH_TIME ms.
    os_timer_arm(&dispatch_timer, DISPATCH_TIME, 1);
}

/**
 * @brief Main entry point and init code.
 */
void ICACHE_FLASH_ATTR user_init(void)
{
	//Register function to run when done.
	system_init_done_cb(start_handler);
    //Turn off auto connect.
    wifi_station_set_auto_connect(false);
    
    // Set baud rate of debug port
    uart_div_modify(0,UART_CLK_FREQ / 115200);
    
#ifndef SDK_DEBUG
	system_set_os_print(false);
#endif

    //os_delay_us(1000);
    
    //Print banner
    db_printf("\nWIFISwitch version %s.\n", STRING_VERSION);
    system_print_meminfo();
    db_printf("Free heap %u\n\n", system_get_free_heap_size());
  
    fs_init();
    
    // Initialize the GPIO subsystem.
    gpio_init();

    //Set GPIO2 to output mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

    //Set GPIO2 low
    gpio_output_set(0, BIT5, BIT5, 0);
    
    db_printf("\nLeaving user_init...\n");
}
