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
	
	debug("Dispatching signal 0x%x.\n", event->sig);
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
			return;
		}
		i++;
		tasks = tasks->next;
	}
	warn("No task handler found for signal 0x%x.\n", event->sig);
}

void task_init(void)
{
	debug("Initialising task handler.\n");
	system_os_task(task_dispatch, TASK_PRIORITY, task_queue, TASK_MAX_QUEUE);
}

int task_add(signal_handler_t handler)
{
	struct task_handler *current_task = task_handlers;
	struct task_handler *tasks = task_handlers;
	struct task_handler *task = NULL;
	
	debug("Adding task handler %p.\n", handler);
	if (!handler)
	{
		warn("No handler.\n");
		return(-1);
	}
	//Check if the task is already there.
	while (current_task)
	{
		if (current_task->handler == handler)
		{
			task = current_task;
			debug(" Task exists.\n");
			break;
		}
		current_task = current_task->next;
	}
	if (!task)
	{
		//No task found.
		debug(" New task.\n");
		//Get mem for entry.
		task = db_zalloc(sizeof(struct task_handler), "task_add task");
		task->signal = (os_signal_t)handler;
		task->handler = handler;
		if (!tasks)
		{
			debug(" First task.\n");
			task_handlers = task;
		}
		else
		{
			debug(" Adding task.\n");
			DL_LIST_ADD_END(task, tasks);
		}
	}
	//Increase reference count.
	task->ref_count++;
	debug(" New reference count %d.\n", task->ref_count);
	//Increase number of tasks.
	n_tasks++;
	debug("%d tasks handlers.\n", n_tasks);
	//Return pointer as ID.
	return(task->signal);
}

struct task_handler *task_remove(os_signal_t signal)
{
	struct task_handler *tasks = task_handlers;
	struct task_handler *task;
	
	debug("Removing task handler %d.\n", signal);
	if (!tasks)
	{
		debug("No handlers registered.\n");
		return(false);
	}
	
	while (tasks)
	{
		task = tasks;
		if (task->signal == signal)
		{
			//Decrease reference count.
			task->ref_count--;
			debug(" New reference count %d.\n", task->ref_count);
			if (!task->ref_count)
			{
				//No more references, delete the task.
				debug("Unlinking handler.\n");
				DL_LIST_UNLINK(task, tasks);	
				n_tasks--;
				debug("%d registered task handlers.\n", n_tasks);
			}
			return(task);
		}
		tasks = tasks->next;
	}
	warn("Handler not fount.\n");
	return(NULL);
}

void task_raise_signal(os_signal_t signal, os_param_t parameter)
{
	system_os_post(TASK_PRIORITY, signal, parameter);
}
