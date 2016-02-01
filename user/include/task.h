/**
 * @file task.h
 *
 * @brief Task related functions.
 * 
 * @ref tasks.md
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
#ifndef TASK_H
#define TASK_H

#include <os_type.h>
#include "task.h"
#include "tools/dl_list.h"

#define TASK_MAX_QUEUE 20
#define TASK_PRIORITY USER_TASK_PRIO_1


/**
 * @brief Type definition of the signal handler callback function pointer.
 */
typedef void (*signal_handler_t)(os_param_t par);

/**
 * @brief Message (parameters) for a task invocation.
 */
struct task_msg
{
	/**
	 * @brief If `TRUE` the memory pointed to by #parameters is freed
	 *        when the task is done.
	 */
	bool free_parm;
	/**
	 * @brief Pointer to the parameters for the task.
	 * 
	 * There is no type, it is up to the task to correctly interpret the
	 * parameters.
	 */
	void *parameters;
	/**
     * @brief Pointers for the prev and next entry in the list.
     */
    DL_LIST_CREATE(struct task_msg);
};

/**
 * @brief Task handler entry.
 */
struct task_handler
{
	/**
	 * @brief Signal to handle.
	 */
	os_signal_t signal;
	/**
	 * @brief Handler function pointer.
	 */
	signal_handler_t handler;
	/**
	 * @brief Reference count.
	 */
	unsigned short ref_count;
	/**
	 * @brief List of messages (parameters) waiting to be handled.
	 */
	struct task_msg *msgs;
	/**
     * @brief Pointers for the prev and next entry in the list.
     */
    DL_LIST_CREATE(struct task_handler);
};

/**
 * @brief Initialise the task system.
 */
extern void task_init(void);

/**
 * @brief Register a task handler.
 *
 * @param handler Function pointer to a handler function.
 * @return Signal number used to call the handler or -1 on error.
 */
extern int task_add(signal_handler_t handler);

/**
 * @brief Remove a registered task handler.
 * 
 * The #task_handler struct is not deallocated.
 * 
 * @param signal Signal of the handler to remove.
 * @return Pointer to the #task_handler struct that was removed.
 */
extern struct task_handler *task_remove(os_signal_t signal);

/**
 * @brief Raise a signal to handle.
 * 
 * @param signal Signal to raise.
 * @param parameters Pointer to parameters for the handler.
 */
extern void task_raise_signal(os_signal_t signal, void *parameters, bool free);
	
#endif //TASK_H
