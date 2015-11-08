/**
 * @file task.c
 *
 * @brief Task related functions.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See thestati
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <stdint.h>
#include "user_interface.h"
#include "user_config.h"
#include "task.h"

static struct task_handler *task_handlers = NULL;

static unsigned int n_tasks = 0;

/**
 * @brief Queue of task messages.
 */
static os_event_t task_queue[TASK_MAX_QUEUE];

static void task_dispatch(os_event_t *event)
{
	struct task_handler *tasks = task_handlers;
	unsigned short i = 1;
	
	debug("Dispatching signal %d.\n", event->sig);
	if (!tasks)
	{
		debug(" No tasks.\n");
		return;
	}

	debug(" %d task handlers.\n", n_tasks);
	while (tasks)
	{
		debug(" Trying handler %d.\n", i);
		if (tasks->signal == event->sig)
		{
			debug("Calling signal handler %p.\n", tasks->handler);
			tasks->handler(event->par);
		}
		i++;
		tasks = tasks->next;
	}
	warn("No task handler found for signal %d.\n", event->sig);
}

void task_init(void)
{
	system_os_task(task_dispatch, TASK_PRIORITY, task_queue, TASK_MAX_QUEUE);
}

void task_add(struct task_handler *task)
{
	struct task_handler *tasks = task_handlers;
	
	debug("Adding task handler %p, for signal %d.\n", task->handler, task->signal);
	if (tasks == NULL)
	{
		tasks = task;
		task->prev = NULL;
		task->next = NULL;
	}
	else
	{
		DL_LIST_ADD_END(task, tasks);
	}
	n_tasks++;
	debug("%d tasks handlers.\n", n_tasks);
}

bool task_remove(struct task_handler *task)
{
	struct task_handler *tasks = task_handlers;
	
	debug("Removing task handler %d.\n", task->signal);
	if (!tasks)
	{
		debug("No handlers registered.\n");
		return(false);
	}
	
	while (tasks)
	{
		if (tasks->signal == task->signal)
		{
			debug("Unlinking handler.\n");
			DL_LIST_UNLINK(task, tasks);	
			n_tasks--;
			debug("%d registered task handlers.\n", n_tasks);
			return(true);
		}
		tasks = tasks->next;
	}
	warn("Handler not fount.\n");
	return(false);
}
