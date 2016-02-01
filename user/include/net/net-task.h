/** @file net-task.h
 *
 * @brief Task interface to network related functions.
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
#ifndef NET_TASK_H
#define NET_TASK_H

#include "task.h"

/**
 * @brief Definition of a type to pass parameters to the send task.
 */
typedef struct net_send_param net_send_t;

/**
 * @brief Struct with data needed for the send task.
 */
struct net_send_param
{
	/**
	 * @brief Data to send.
	 */
	char *data;
	/**
	 * @brief Length of data to be send.
	 */
	size_t len;
	/**
	 * @brief Connection to use when sending.
	 */
	struct net_connection *connection;
};

/**
 * @brief Register network tasks.
 */
void net_register_tasks(void);

#endif //NET_TASK_H
