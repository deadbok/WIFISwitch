/**
 * @file task.c
 *
 * @brief Task related functions.
 * 
 * See [Task handler system](tasks.md).
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
#include "debug.h"
#include "task.h"

static struct task_handler *task_handlers = NULL;

static unsigned int n_tasks = 0;

/**
 * @brief Queue of task messages.
 */
static os_event_t task_queue[TASK_MAX_QUEUE];

/**
 * @brief Get task handler by signal.
 * 
 * @param signal The signal for which to find the handler.
 * @return Pointer to the #task_handler struct.
 */
static struct task_handler *task_get_handler(os_signal_t signal)
{
	struct task_handler *tasks = task_handlers;
	
	debug("Finding task handler for signal %d.\n", signal);
	if (!tasks)
	{
		debug(" No tasks.\n");
		return(NULL);
	}
	while (tasks)
	{
		if (tasks->signal == signal)
		{
			debug(" Found %p.\n", tasks);
			return(tasks);
		}
		tasks = tasks->next;
	}
	debug(" Not found.\n");
	return(NULL);
}
	
/**
 * @brief General task dispatcher called by the SDK.
 * 
 * Finds the right handler and sends the first waiting parameters.
 */
static void task_dispatch(os_event_t *event)
{
	struct task_handler *task = NULL;
	struct task_msg *msg = NULL;
	
	debug("Dispatching signal 0x%x.\n", event->sig);
	debug(" %d task handler(s).\n", n_tasks);
	task = task_get_handler(event->sig);
	if (task)
	{
		//Grab the first waiting message.
		if (task->msgs)
		{
			msg = task->msgs;
			debug(" Calling signal handler %p.\n", task->handler);
			task->handler((os_param_t) msg->parameters);
			if (msg->free_parm)
			{
				debug(" Freeing parameter data at %p.\n",
					  msg->parameters);
				db_free(msg->parameters);
			}
		}
		else
		{
			warn("No messages waiting for the task.");
		}
	}
	else
	{
		warn("No task handler found for signal 0x%x.\n", event->sig);
	}
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
			//Get out of the loop.
			break;
		}
		current_task = current_task->next;
	}
	if (!task)
	{
		//No task found.
		debug(" New task.\n");
		//Get mem for entry, and zero to make sure we have NULL pointers.
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
	debug("%d tasks handler(s).\n", n_tasks);
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

void task_raise_signal(os_signal_t signal, void *parameters, bool free)
{
	struct task_handler *task = NULL;
	struct task_msg *msgs = NULL;
	struct task_msg *new_msg = NULL;
	
	debug("Signalling %d with parameters at %p.\n", signal, parameters);
	task = task_get_handler(signal);
	if (!task)
	{
		error(" No task, quitting.\n");
		return;
	}
	else
	{
		new_msg = db_zalloc(sizeof(struct task_msg), "task_raise_signal new_msg");
		new_msg->free_parm = free;
		new_msg->parameters = parameters;
		if (task->msgs)
		{
			msgs = task->msgs;
			DL_LIST_ADD_END(new_msg, msgs);
		}
		else
		{
			task->msgs = new_msg;
		}
	}
	system_os_post(TASK_PRIORITY, signal, 0);
}
