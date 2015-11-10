/**
 * @file button.h
 *
 * @brief Physical button handling routines.
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
#ifndef BUTTON_H
#define BUTTON_H

#include "task.h"

/**
 * @brief Number of possible buttons.
 */
#define BUTTONS_MAX 16
/**
 * @brief De-bounce time in microseconds.
 */
#define BUTTONS_DEBOUNCE_US 10000

/**
 * @brief Button callback handler type.
 */
typedef signal_handler_t button_handler_t;

/**
 * @brief Struct to hold button information.
 */
struct button_info
{
	/**
	 * @brief Status of the button.
	 */
	bool enabled;
	/**
	 * @brief Number of microseconds that the button was pressed.
	 */
	unsigned int time;
	/**
	 * @brief Signal used to call the button handler.
	 */
	os_signal_t signal;
	GPIO_INT_TYPE edge;
};

/**
 * @brief Array of all buttons.
 */
extern struct button_info buttons[];

extern void button_map(uint32_t mux, uint32_t func, unsigned char gpio, button_handler_t handler);
extern void button_unmap(unsigned char gpio);
/**
 * @brief Initialise button handling.
 * 
 * *gpio_init() and task_init() must have been called first.*
 */
extern void button_init(void);
/**
 * @brief Call this from the button handler to acknowledge that the signal has been handled.
 */
extern void button_ack(unsigned char gpio);
#endif //BUTTON_H
