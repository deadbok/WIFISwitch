/**
 *
 * @file user_main.c
 *
 * @brief Main program file.
 * 
 * Main task functions.
 * --------------------
 *
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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "main.h"
#include "fwconf.h"
#include "debug.h"
#include "fs/fs.h"
#include "driver/button.h"
#include "config/config.h"

/**
 * @brief Linker symbol that points to the end of the ROM code in flash.
 */
extern uint32_t _irom0_text_end;

/**
 * @brief Configuration read from the flash.
 */
struct config *cfg = NULL;

/**
 * @brief Config mode enabler.
 */
static bool config_mode = false;

/**
 * @brief Config mode enabler.
 */
static xQueueHandle main_queue;

/**
 * @brief Status task handle.
 */
xTaskHandle status_handle;
xTaskHandle main_handle;

/**
 * @brief Button handler that activates configuration mode. 
 * 
 * *Only used until a network connection has been made.*
 * 
 * @param gpio The gpio that triggered the handler.
 */
static void button_config(uint32_t gpio)
{
	debug("Button press %d.\n", gpio);
	
	config_mode = true;
	printf("Configuration mode enabled.\n");
	button_ack(gpio);
}

/**
 * @brief Button handler that switches the relay.
 * 
 * @param gpio The gpio that triggered the handler.
 */
static void button_switch(uint32_t gpio)
{
	uint32_t msg = MAIN_RESET;
	
	debug("Button press %d.\n", gpio);
	
	if (buttons[gpio].time > 5000000)
	{
		debug(" Long press. ");
		printf("Resetting on request from hardware button.\n");
		xQueueSend(main_queue, &msg, 1000);
	}
	else
	{
		debug(" Switching output state.\n");
		gpio_toggle(5);
		//ws_wifiswitch_send_gpio_status();
	}
	button_ack(gpio);
}

/**
 * @brief Called by the timer to check the status and connection time out.
 * 
 * Print memory and connection infos when in debug mode. Handle time out
 * of TCP connections.
 */
static void status_task(void *pvParameters)
{
	debug("Status task.\n");
	while(1)
	{
		debug("Alive.\n");
		vTaskDelay(CHECK_TIME / portTICK_RATE_MS);
	}
}

/**
 * @brief Main task.
 */
static void main_task(void *pvParameters)
{
	uint32_t msg;
	
	debug("Main task.\n");  
    while(1)
    {
        uint32_t signal;
        
        debug(" Checking messages.\n");
        if (xQueueReceive(main_queue, &signal, portMAX_DELAY))
        {
			switch(signal)
			{
				case MAIN_HALT:
					printf("Halting system...\n");
					vTaskSuspendAll();
					break;
				case MAIN_RESET:
					printf("Restarting...\n");
					sdk_system_restart();
					break;
				case MAIN_INIT:
					debug("Init...\n");
					cfg = read_cfg_flash();
					if (cfg)
					{
						//Initialise button handler.
						button_init();
						
						//Wait for config button.
						printf("\nPress the button now to enter configuration mode.\n");
						printf("Waiting %d second(s)...\n\n", CHECK_TIME / 1000);
						vTaskDelay(CHECK_TIME / portTICK_RATE_MS);

						//Map the button to a GPIO and switch mode.
						button_map(SWITCH_KEY_NUM, button_switch);

						//Initialise file system.
						fs_init();
						
						//Create status task.
						xTaskCreate(status_task, (signed char *)"status", 256, NULL, tskIDLE_PRIORITY, status_handle);
						debug("Status handle %p.\n", status_handle);
					}
					else
					{
						debug("Could not load configuration.\n");
						msg = MAIN_HALT;
						xQueueSendToBack(main_queue, &msg, 1000);
					}
					break;
				default:
					warn("Unknown signal: %d.\n", signal);
			}
        }
		else
		{
			debug("Nothing was received.\n");
		}
    }
}

/**
 * @brief Main entry point and init code.
 */
void user_init(void)
{
	uint32_t msg = MAIN_INIT;
	
    //Turn off auto connect.
    sdk_wifi_station_set_auto_connect(false);
    //Turn off auto reconnect.
    //sdk_wifi_station_set_reconnect_policy(false);

    //Set baud rate of debug port
    //uart_div_modify(0,UART_CLK_FREQ / BAUD_RATE);
    uart_set_baud(0, BAUD_RATE);

    //Print banner
    printf("\n%s version %s (%s).\n", PROJECT_NAME, PROJECT_VERSION, GIT_VERSION);
    printf("SDK version %s.\n", sdk_system_get_sdk_version());
    printf("Free heap %u\n", sdk_system_get_free_heap_size());
    debug("ROM firmware portion ends at %p.\n", &_irom0_text_end);
    
    //Initialise task system.
    debug("Creating tasks...\n");
    main_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(main_task, (signed char *)"main", 256, main_queue, PRIO_MAIN, main_handle);
    debug("Status handle %p.\n", main_handle);
    
    //Map the button to set config mode.
    button_map(SWITCH_KEY_NUM, button_config);
    
    xQueueSendToBack(main_queue, &msg, 1000);
}
