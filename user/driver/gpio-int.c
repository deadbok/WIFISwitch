/**
 * @file gpio-int.c
 *
 * @brief GPIO interrupt handler code.
 *
 * @copyright
 * Copyright 2016 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
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
#include <stdio.h>
#include "esp/uart.h"
#include "esp/gpio.h"
#include "debug.h"
#include "driver/button.h"
 
void IRAM gpio_interrupt_handler(void)
{
    uint32_t status_reg = GPIO.STATUS;
    
    debug("GPIO interrupt, status: %d.\n", GPIO.STATUS);
    
	button_intr_handler();
	
	GPIO.STATUS_CLEAR = status_reg;
}


