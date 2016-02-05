/**
 * @file button.c
 *
 * @brief Physical button handling routines.
 * 
 * *Button GPIOS are pulled up by default. When the button is activated
 * the GPIO goes low.*
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
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "main.h"
#include "fwconf.h"
#include "debug.h"
#include "driver/button.h"
#include "driver/gpio-int.h"

struct button_info buttons[BUTTONS_MAX] = {
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL},
	{false, 0, NULL}
};

/**
 * @brief Queue used to communicate with the button task.
 */
static xQueueHandle button_queue;

/**
 * @brief Button task handle.
 */
xTaskHandle button_handle;

void IRAM button_intr_handler(void)
{
	unsigned int start_time;
	uint32_t i;
	uint32_t pin_mask;
	/* We have not woken a task at the start of the ISR. */
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	debug("Button interrupt handler.\n");
	debug(" GPIO interrupt mask 0x%x.\n", GPIO.STATUS);
	debug(" Status 0x%x.\n", GPIO.IN);
	
	//Get system time in ms.
	start_time = sdk_system_get_time();
	
	debug(" GPIO states 0x%x.\n", GPIO.STATUS);
	
	//Check each GPIO for an interrupt.
	for (i = 0; i < 16; i++)
	{
		pin_mask =  1 << i;
		if (GPIO.STATUS & pin_mask)
		{
			debug(" Button: %d.\n", i);
			if (buttons[i].enabled)
			{
				debug(" Activation time %d.\n", buttons[i].time);
				debug(" Current time %d.\n", start_time);
				if ((buttons[i].time == 0))
				{
					debug(" New press at %d.\n", start_time);
					buttons[i].time = start_time;
					//Interrupt on positive GPIO edge (button release).
					gpio_set_interrupt(i, GPIO_INTTYPE_EDGE_POS);
				}
				else if (((buttons[i].time + BUTTONS_DEBOUNCE_US) < start_time))
				{
					buttons[i].time = start_time - buttons[i].time;
					debug(" Release after %dus.\n", buttons[i].time);
					//Raise button signal.
					xQueueSendToFrontFromISR(button_queue, &i, &xHigherPriorityTaskWoken);
					//Interrupt on negative GPIO edge (button press).
					gpio_set_interrupt(i, GPIO_INTTYPE_EDGE_NEG);
				}
				else
				{
					debug(" State change within de-bounce period, ignoring.\n");
					/* Wait for a positive edge (button release), even
					 * though it may already have occurred.
					 */
					gpio_set_interrupt(i, GPIO_INTTYPE_EDGE_POS);
				}
				if (xHigherPriorityTaskWoken)
				{
					taskYIELD();
				}
			}
			else
			{
				warn("Not enabled, skipping.\n");
			}
		}
	}
}

void button_map(unsigned char gpio, button_handler_t handler)
{
	debug("Mapping button at GPIO %d.\n", gpio);
	
	if ((gpio + 1) > BUTTONS_MAX)
	{
		warn("GPIO does not exist.\n");
		return;
	}
	//Set GPIO function.
	//Enable pullup.
	gpio_enable(gpio, GPIO_INPUT_PULLUP);
	
	//Keep track of enabled keys.
	buttons[gpio].enabled = true;
	
	//Reset de-bounce time.
	buttons[gpio].time = 0;
	
	//Handler.
	if (buttons[gpio].handler)
	{
		debug(" Replacing handler: %p.\n", buttons[gpio].handler);
	}
	buttons[gpio].handler = handler;
	
	gpio_set_interrupt(gpio, GPIO_INTTYPE_EDGE_NEG);
}

void button_unmap(unsigned char gpio)
{
	debug("Un-mapping button at GPIO %d.\n", gpio);
	
	//Disable task.
	if ((gpio + 1) > BUTTONS_MAX)
	{
		warn("GPIO does not exist.\n");
		return;
	}
	buttons[gpio].enabled = false;
	buttons[gpio].handler = NULL;
}

static void button_task(void *pvParameters)
{
	debug("Button task.\n");
	  
    while(1)
    {
        uint32_t gpio;

        if (xQueueReceive(button_queue, &gpio, portMAX_DELAY))
        {
			if (gpio < 16)
			{
				debug(" Calling GPIO %d handler.\n", gpio);
				buttons[gpio].handler(gpio);
			}
        }
		else
		{
			debug("Nothing was received.\n");
		}
    }

}

void button_init(void)
{
	debug("Creating button task.\n");
	button_queue = xQueueCreate(5, sizeof(uint32_t));
	xTaskCreate(button_task, (signed char *)"button", 256, button_queue, PRIO_BUTTON, button_handle);
	debug("Button handle %p.\n", button_handle);
}	

void button_ack(unsigned char gpio)
{
	debug("Button action on GPIO %d acknowledged.\n", gpio);
	buttons[gpio].time = 0;
}
