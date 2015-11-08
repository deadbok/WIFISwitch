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
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "task.h"
#include "driver/uart.h"
#include "fs/fs.h"
#include "net/wifi.h"
#include "net/udp.h"
#include "net/tcp.h"
#include "net/capport.h"
#include "slighttp/http.h"
#include "driver/button.h"
#include "handlers/rest/rest.h"
#include "handlers/fs//http-fs.h"
#include "slighttp/http-handler.h"
#include "handlers/deny/http-deny.h"

extern uint32_t _irom0_text_end;

struct config *cfg;

/**
 * @brief Timer for handling events.
 */
os_timer_t status_timer;

/**
 * @brief Called by the timer to check the status.
 */
static void status_check(void)
{
	db_mem_list();
	udp_print_connection_status();
	tcp_print_connection_status();
}

/**
 * @brief Called when the WIFI is connected or in AP mode.
 */
static void connected(unsigned char mode)
{
	//Start status task.
	//Disarm timer.
	os_timer_disarm(&status_timer);
	//Setup timer, pass call back as parameter.
	os_timer_setfn(&status_timer, (os_timer_func_t *)status_check, NULL);
	
	if (mode < 2)
	{
		//Start web server with default pages.
		init_http(80);		
		db_printf("Adding memory REST handler.\n");
		http_add_handler("/rest/fw/mem", &http_rest_mem_handler);
		db_printf("Adding version REST handler.\n");
		http_add_handler("/rest/fw/version", &http_rest_version_handler);
		/*db_printf("Adding network names REST handler.\n");
		http_add_handler("/rest/net/networks", &http_rest_net_names_handler);
		db_printf("Adding default network REST handler.\n");
		http_add_handler("/rest/net/network", &http_rest_network_handler);*/
		db_printf("Adding gpio REST handler.\n");
		http_add_handler("/rest/gpios/*", &http_rest_gpio_handler);
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
		//Start captive portal
		init_captive_portal("wifiswitch");
		//Start in network configuration mode.
		init_http(80);
		db_printf("Adding memory REST handler.\n");
		http_add_handler("/rest/fw/mem", &http_rest_mem_handler);
		db_printf("Adding version REST handler.\n");
		http_add_handler("/rest/fw/version", &http_rest_version_handler);
		db_printf("Adding network password REST handler.\n");
		http_add_handler("/rest/net/password", &http_rest_net_passwd_handler);
		db_printf("Adding network names REST handler.\n");
		http_add_handler("/rest/net/networks", &http_rest_net_names_handler);
		db_printf("Adding default network REST handler.\n");
		http_add_handler("/rest/net/network", &http_rest_network_handler);
		http_fs_init("/connect/");
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
 static void disconnected(void)
 {
	if (http_get_status())
	{
		db_printf("Network connection lost, restarting...\n");
		system_restart();
	}
 }
 
/**
 * @brief Called when initialisation is over.
 *
 * Starts the connection task.
 */
static void start_connection(void)
{
    db_printf("Running...\n");
    task_init();
    wifi_init("wifiswitch", connected, disconnected);
}

/**
 * @brief Main entry point and init code.
 */
void user_init(void)
{
    //Register function to run when done.
    system_init_done_cb(start_connection);
    //Turn off auto connect.
    wifi_station_set_auto_connect(false);
    //Turn off auto reconnect.
    wifi_station_set_reconnect_policy(false);

    // Set baud rate of debug port
    //uart_div_modify(0,UART_CLK_FREQ / BAUD_RATE);
    uart_init(BAUD_RATE, BAUD_RATE);

#ifndef SDK_DEBUG
	system_set_os_print(false);
#endif

    //os_delay_us(1000);

    //Print banner
    db_printf("\n%s version %s (%s).\n", PROJECT_NAME, VERSION, GIT_VERSION);
    //system_print_meminfo();
    db_printf("SDK version %s.\n", system_get_sdk_version());
    db_printf("Free heap %u\n", system_get_free_heap_size());
    db_printf("ROM firmware portion ends at %p.\n", &_irom0_text_end);

	cfg = read_cfg_flash();

    fs_init();

    // Initialize the GPIO subsystem.
    gpio_init();

	//Map the button to a GPIO);
    button_map (SWITCH_KEY_NAME, SWITCH_KEY_FUNC, SWITCH_KEY_NUM);

    //Set GPIO5 function.
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

    //Set GPIO5 to output.
    gpio_output_set(0, GPIO_ID_PIN(5), GPIO_ID_PIN(5), 0);

    db_printf("\nLeaving user_init...\n");
}
