/** @file net-task.c
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
#include "user_config.h"
#include "net/net.h"
#include "debug.h"
#include "task.h"

os_signal_t signal_net_disconnect;
os_signal_t signal_net_send;

static void net_task_send(os_signal_t net_send_t_addr)
{
	net_send_t *net_send_data = (net_send_t *)net_send_t_addr;
	size_t ret;
	
	debug("Net send task.\n");
	
	if (!net_send_data)
	{
		warn("Nothing to send.\n");
		return;
	}

	ret = net_send(net_send_data->data, net_send_data->len, net_send_data->connection->conn);
	debug(" %d bytes send.\n", ret);
}

static void net_task_disconnect(os_signal_t con_addr)
{
	struct net_connection *connection = (struct net_connection *)con_addr;
	
	debug("Net disconnect task.\n");

	if (!connection)
	{
		warn("No connection.");
		return;
	}
	net_disconnect(connection);
}

void net_register_tasks(void)
{
	debug("Registering network tasks.\n");
	
	//Register reset handler.
    signal_net_disconnect = task_add(net_task_disconnect);
    signal_net_send = task_add(net_task_send);
}
