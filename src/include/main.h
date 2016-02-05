/** @file main.h
 *
 * @brief Main routine stuff.
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
#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "config/config.h"

enum task_prios
{
	PRIO_IDLE,
	PRIO_MAIN, 
	PRIO_BUTTON
};

enum main_signal
{
	MAIN_HALT,
	MAIN_RESET,
	MAIN_INIT,
	MAIN_WIFI,
	MAIN_NET,
	MAIN_SERVER,
	N_MAIN_SIGNALS
};

/**
 * @brief Firmware configuration, loaded from flash.
 */
extern struct config *cfg;

#endif //MAIN_H
