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
 * driven. The WIFI functions do not use call backs (fixed in a later SDK
 * version), while the TCP functions do.
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
 * The SDK states that you can not send another TCP packet, before the call back
 * has signalled that the current one is done. All function sending TCP must
 * therefore be able to split the send data, over consecutive calls. 
 */
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include "user_config.h"
#include "task.h"
#include "driver/uart.h"
#include "driver/button.h"
#include "fs/fs.h"
#include "net/wifi.h"
#include "net/udp.h"
#include "net/tcp.h"
#include "net/capport.h"
#include "slighttp/http.h"
#include "driver/button.h"
#include "slighttp/http-handler.h"
#include "handlers/http/fs//http-fs.h"
#include "handlers/http/deny/http-deny.h"
#include "handlers/http/websocket/websocket.h"
#include "handlers/websocket/wifiswitch.h"

/**
 * @brief Linker symbol that points to the end of the ROM code in flash.
 */
extern uint32_t _irom0_text_end;

/**
 * @brief Configuration read from the flash.
 */
struct config *cfg;

/**
 * @brief Config mode enabler.
 */
static bool config_mode = false;

/**
 * @brief Signal to de a system reset.
 */
os_signal_t signal_reset;

/**
 * @brief Timer for handling events.
 */
os_timer_t status_timer;

/**
 * @brief Button handler that activates configuration mode. 
 * 
 * *Only used until a network connection has been made.*
 * 
 * @param gpio The gpio that triggered the handler.
 */
static void button_config(os_param_t gpio)
{
	debug("Button press %d.\n", gpio);
	config_mode = true;
	db_printf("Configuration mode enabled.\n");
	
	button_ack(gpio);
}

/**
 * @brief Button handler that switches the relay.
 * 
 * @param gpio The gpio that triggered the handler.
 */
static void button_switch(os_param_t gpio)
{
	unsigned int gpio_state;
	
	debug("Button press %d.\n", gpio);
	if (buttons[gpio].time > 5000000)
	{
		debug(" Long press. ");
		db_printf("Resetting on request from hardware button.\n");
		task_raise_signal(signal_reset, 0);
	}
	else
	{
		debug(" Switching output state.\n");
		gpio_state = !GPIO_INPUT_GET(5);
		debug(" New state: %d.\n", gpio_state);
		GPIO_OUTPUT_SET(5, gpio_state);
		ws_wifiswitch_send_gpio_status();
	}
	button_ack(gpio);
}

/**
 * @brief Called by the timer to check the status and connection time out.
 * 
 * Print memory and connection infos when in debug mode. Handle time out
 * of TCP connections.
 */
static void status_check(void)
{
	struct net_connection *connections;
	
	db_mem_list();
	udp_print_connection_status();
	tcp_print_connection_status();
	
	//Time out handling of TCP connections.
	connections = tcp_get_connections();
	while (connections)
	{
		connections->inactivity += CHECK_TIME;
		debug(" Connection %p milliseconds without activity %d.\n", connections, connections->inactivity);
		if ((connections->timeout > 0) && (connections->timeout < connections->inactivity))
		{
			debug(" Connection time out.\n");
			if (connections->ctrlfuncs->close)
			{
				debug(" Closing using control function at %p.\n", connections->ctrlfuncs->close);
				connections->ctrlfuncs->close(connections);
			}
		}
		connections = connections->next;
	}
	if (net_is_sending())
	{
		debug(" Someone is sending something.\n");
	}
}

/**
 * @brief Called when the WIFI is connected or in AP mode.
 */
static void connected(os_signal_t mode)
{
	char *hostname = NULL;
	
	//Map the button to a GPIO and relay switch.
    button_map(SWITCH_KEY_NAME, SWITCH_KEY_FUNC, SWITCH_KEY_NUM, button_switch);
	//Start status task.
	//Disarm timer.
	os_timer_disarm(&status_timer);
	//Setup timer, pass call back as parameter.
	os_timer_setfn(&status_timer, (os_timer_func_t *)status_check, NULL);
	
	//Setup WebSocket handler.
	init_ws();
	if (!ws_register_wifiswitch())
	{
		error(" Could not register wifiswitch webSocket protocol.\n");
	}
	
	if (cfg->network_mode == WIFI_MODE_AP)
	{
		hostname = wifi_station_get_hostname();
		if (!hostname)
		{
			error("Could not get host name.\n");
			return;
		}
		//Start captive portal in AP mode.
		init_captive_portal(hostname);
	}
	
	if (!config_mode)
	{
		//Start web server with default pages.
		init_http(80);		
		db_printf("Adding WebSocket handler.\n");
		http_add_handler("/ws*", &http_ws_handler);
		db_printf("Adding deny handler.\n");
		http_add_handler("/connect/*", &http_deny_handler);
		http_fs_init("/");
		db_printf("Adding file system handler.\n");
		http_add_handler("/*", &http_fs_handler);
		db_printf("Adding file system error status handler.\n");
		http_add_handler("/*", &http_fs_error_handler);
		db_printf("Adding error status handler.\n");
		http_add_handler("/*", &http_status_handler);
	}
	else
	{
		//Start in network configuration mode.
		init_http(80);
		http_fs_init("/connect/");
		db_printf("Adding WebSocket handler.\n");
		http_add_handler("/ws*", &http_ws_handler);
		db_printf("Adding file system handler.\n");
		http_add_handler("/*", &http_fs_handler);
		db_printf("Adding file system error status handler.\n");
		http_add_handler("/*", &http_fs_error_handler);
		db_printf("Adding error status handler.\n");
		http_add_handler("/*", &http_status_handler);
	}
	//Arm the timer, run every #CHECK_TIME  ms.
	os_timer_arm(&status_timer, CHECK_TIME, 1);
}

/**
 * @brief Disconnect handler.
 * 
 * Simply reset the chip.
 * @todo Stop HTTP server instead.
 */
static void disconnected(os_signal_t signal)
{
	db_printf("Network connection lost, restarting...\n");
	system_restart();
}
 
 /**
 * @brief Handler to do a system reset,
 */
static void reset(os_param_t dummy)
{
	db_printf("Restarting...\n");
	system_restart();
}
 
/**
 * @brief Called when initialisation is over.
 *
 * Starts the connection task.
 */
static void start_connection(void)
{
    db_printf("Running...\n");
    //Register reset handler.
    signal_reset = task_add(reset);
    //Connect to a configured WiFi or create an AP.
    wifi_init("wifiswitch", connected, disconnected);
}

/**
 * @brief Start waiting for configuration mode select.
 * 
 * This is called after init, and waits #CHECK_TIME for somebody to
 * press the button to enter configuration mode.
 */
static void wait_config(void)
{
	db_printf("Waiting %d second(s)...\n", CHECK_TIME / 1000);
	//Disarm timer.
	os_timer_disarm(&status_timer);
	//Setup timer, pass call back as parameter.
	os_timer_setfn(&status_timer, (os_timer_func_t *)start_connection, NULL);
	//Arm the timer, run every #CHECK_TIME  ms.
	os_timer_arm(&status_timer, CHECK_TIME, 1);
}

/**
 * @brief Main entry point and init code.
 */
void user_init(void)
{
    //Register function to run when done.
    system_init_done_cb(wait_config);
    //Turn off auto connect.
    wifi_station_set_auto_connect(false);
    //Turn off auto reconnect.
    wifi_station_set_reconnect_policy(false);

    //Set baud rate of debug port
    //uart_div_modify(0,UART_CLK_FREQ / BAUD_RATE);
    uart_init(BAUD_RATE, BAUD_RATE);

#ifndef SDK_DEBUG
	system_set_os_print(false);
#endif
    //Print banner
    db_printf("\n%s version %s (%s).\n", PROJECT_NAME, VERSION, GIT_VERSION);
    db_printf("SDK version %s.\n", system_get_sdk_version());
    db_printf("Free heap %u\n", system_get_free_heap_size());
    debug("ROM firmware portion ends at %p.\n", &_irom0_text_end);

	cfg = read_cfg_flash();

    //Initialise the GPIO subsystem.
    gpio_init();
    debug("GPIO states 0x%x.\n", GPIO_REG_READ(GPIO_OUT_ADDRESS));
    
    //Initialise task system.
    task_init();
    
    //Initialise button handler.
    button_init();
    
    db_printf("\nPress the button now to enter configuration mode.\n\n");

	//Map the button to a GPIO and config mode switch.
    button_map(SWITCH_KEY_NAME, SWITCH_KEY_FUNC, SWITCH_KEY_NUM, button_config);

    //Set GPIO5 function.
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

    //Set GPIO5 to output.
    gpio_output_set(0, GPIO_ID_PIN(5), GPIO_ID_PIN(5), 0);

	//Initialise file system.
    fs_init();

    debug("Leaving user_init...\n");
}
