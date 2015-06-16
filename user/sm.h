/** @file sm.h
 *
 * @brief Main state machine stuff.
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
#ifndef SM_H
#define SM_H

#include "sm_types.h"

/**
 * @brief Number of states in the event dispatcher.
 */
#define N_STATES 9

extern state_t wifi_connect(void *context);
extern state_t wifi_connected(void *context);

extern state_t http_init(void *arg);
extern state_t http_send(void *arg);
extern state_t http_mutter_med_kost_og_spand(void*arg);

extern state_t root_config_mode(void *arg);
extern state_t root_normal_mode(void *arg);
extern state_t reboot(void *arg);

/**
 * @brief Array of pointer to functions that will handle event.
 * 
 * This array is used to look up the current handler for an event. Events should
 * be ordered to handle the event of the same place in the #states enum.
 */
static state_handler_t handlers[N_STATES] = 
{
	wifi_connect, 
	wifi_connected,
	root_normal_mode,
	root_config_mode,
	NULL,
	http_init,
	http_send,
	http_mutter_med_kost_og_spand,
	reboot
};



#endif //SM_H
