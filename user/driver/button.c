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
#include "user_config.h"
#include "gpio.h"
#include "user_interface.h"
#include "driver/button.h"

struct button_info buttons[BUTTONS_MAX];

static void button_intr_handler(uint32_t mask, void *arg)
{
	unsigned int start_time;
	uint32_t gpio_status;
	unsigned char i;
	uint16_t pin_mask;
	
	debug("Button interrupt handler.\n");
	debug(" GPIO interrupt mask 0x%x.\n", mask);
	
	//gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	//debug(" Status 0x%x.\n", gpio_status);
	
	//Get system time in ms.
	start_time = system_get_time();
	
	debug(" GPIO states 0x%x.\n", GPIO_REG_READ(GPIO_OUT_ADDRESS));
	
	//Check each GPIO for an interrupt.
	for (i = 0; i < 16; i++)
	{
		pin_mask =  1 << i;
		if (mask & pin_mask)
		{
			if (buttons[i].enabled)
			{
				debug(" Activation time %d.\n", buttons[i].time);
				debug(" Current time %d.\n", start_time);
				if ((buttons[i].time == 0))
				{
					debug(" New press at %d.\n", start_time);
					buttons[i].time = start_time;
					//Interrupt on positive GPIO edge (button release).
					buttons[i].edge = GPIO_PIN_INTR_POSEDGE;
				}
				else if (((buttons[i].time + BUTTONS_DEBOUNCE_US) < start_time))
				{
					buttons[i].time = start_time - buttons[i].time;
					debug(" Release after %dus.\n", buttons[i].time);
					//Raise button signal.
					task_raise_signal(buttons[i].signal, i);
					//Interrupt on negative GPIO edge (button press).
					buttons[i].edge = GPIO_PIN_INTR_NEGEDGE;
				}
				else
				{
					debug(" State change within de-bounce period, ignoring.\n");
					/* Wait for a positive edge (button release), even
					 * though it may already have occurred.
					 */
					buttons[i].edge = GPIO_PIN_INTR_POSEDGE;
				}
			}
			else
			{
				warn("Not enabled, skipping.\n");
			}
		}
	}
	//This seems to not work if called after gpio_pin_intr_state_set.
	gpio_intr_ack(mask);
	for (i = 0; i < 16; i++)
	{
		gpio_pin_intr_state_set(GPIO_ID_PIN(i), buttons[i].edge);
	}
}

void button_map(uint32_t mux, uint32_t func, unsigned char gpio, button_handler_t handler)
{
	debug("Mapping button at GPIO %d.\n", gpio);
	
	if ((gpio + 1) > BUTTONS_MAX)
	{
		warn("GPIO does not exist.\n");
		return;
	}
	//Set GPIO function.
    PIN_FUNC_SELECT(mux, func);
    
    //Enable pullup.
    PIN_PULLUP_EN(mux);
	
	//Keep track enabled keys.
	buttons[gpio].enabled = true;
	
	//Reset de-bounce time.
	buttons[gpio].time = 0;
	
	//Set interrupt on falling edge.
	buttons[gpio].edge = GPIO_PIN_INTR_NEGEDGE;
	
	//Set signal.
	buttons[gpio].signal = task_add(handler);
	debug(" Registered handler signal 0x%x.\n", buttons[gpio].signal);
	
	//Set GPIO as input
    gpio_output_set(0, 0, 0, GPIO_ID_PIN(4));
    
	//Disable interrupts on GPIO's before messing with them.
	ETS_GPIO_INTR_DISABLE();
	
	//Interrupt on any GPIO edge.
    //gpio_pin_intr_state_set(GPIO_ID_PIN(gpio), GPIO_PIN_INTR_ANYEDGE);
    gpio_pin_intr_state_set(GPIO_ID_PIN(gpio), buttons[gpio].edge);
    
    //Clear GPIO status.
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(gpio));
	
	//We're done turn ints on again. 
	ETS_GPIO_INTR_ENABLE();
}

void button_unmap(unsigned char gpio)
{
	struct task_handler *handler;
	debug("Un-mapping button at GPIO %d.\n", gpio);
	
	//Disable task.
	if ((gpio + 1) > BUTTONS_MAX)
	{
		warn("GPIO does not exist.\n");
		return;
	}
	buttons[gpio].enabled = false;
	
	handler = task_remove(buttons[gpio].signal);
	if (!handler->ref_count)
	{
		debug(" Freeing task %p.\n", handler);
		db_free(handler);
	}
}

void button_init(void)
{
	//Disable interrupts on GPIO's before messing with them.
	ETS_GPIO_INTR_DISABLE();
    
    //Attach interrupt function.
	gpio_intr_handler_register(button_intr_handler, NULL);
	
	//We're done turn ints on again. 
	ETS_GPIO_INTR_ENABLE();
}	

void button_ack(unsigned char gpio)
{
	debug("Button action on GPIO %d acknowledged.\n", gpio);
	buttons[gpio].time = 0;
}
