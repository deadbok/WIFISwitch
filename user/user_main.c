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
 *  
 * Architecture.
 * =============
 * 
 * The ESP8266 SDK is a strange beast, it seems a mix of procedural and event
 * driven. The WIFI functions do not use call backs, while the TCP functions
 * do. A little consistency would be nice here, a call back when the connection
 * is lost? But no. Oh and there are bugs, at least that's how I see them, like
 * the TCP disconnect call back, getting passed a mostly unusable parameter.
 * 
 * After trying to force a procedural scheme on it all, and chasing the same
 * shortcoming into a corner, I finally realised that, I was working against
 * the SDK.
 * The SDK seems to run in the same single task, as the user code. This has
 * the implication, that if you eat up all cycles, nothing gets done in the
 * SDK. If you do this for to long, the watch dog timer will kick in, and
 * reset the chip. You have to leave your functions, for the SDK to take over
 * processing. In my case, I would parse a HTTP request, and assemble the
 * response in one large go, then leave the control to the CPU. This sorta
 * worked. But what happens if the response is larger than the SDK TCP buffer,
 * or a new request is received before the sending is done, and it all ends
 * up with one more response to send. Stuff will get lost, and things will not
 * behave as expected.
 *   
 * I ended up dreaming about my Delphi days (something that I almost never do),
 * and that call, that you could make to get the system to do some processing,
 * while you where in a long chunk of code; But in the SDK there is none.
 * Instead no user code may run for more than 10ms, the docs says. To
 * accomplish this, a state machine seems a good solution. The state machine
 * is triggered at regular intervals using the one timer available. The TCP
 * request is still handled in the receive call back, but the sending code
 * is running in the state machine, which allows me to split the response
 * into chunks, and give back control to the SDK between sending each one.
 */
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
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
	else
	{
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
		else
		{
			db_printf("Tick!\n");
		}
#endif

		if (state >= N_MAIN_STATES)
		{
			error("Main state machine state overflow.");
			state = 0;
		}
		go_away = false;
	}
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
    //Turn off auto reconnect.
    wifi_station_set_reconnect_policy(false);
    
    // Set baud rate of debug port
    uart_div_modify(0,UART_CLK_FREQ / 115200);
    //uart_init(115200, 115200);
    
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
