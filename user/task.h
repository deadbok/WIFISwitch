/**
 * @file task.h
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
#ifndef TASK_H
#define TASK_H

#include "tools/dl_list.h"

#define TASK_MAX_QUEUE 20
#define TASK_PRIORITY USER_TASK_PRIO_1

typedef void (*signal_handler_t)(os_param_t par);

struct task_handler
{
	os_signal_t signal;
	signal_handler_t handler;
	/**
     * @brief Pointers for the prev and next entry in the list.
     */
    DL_LIST_CREATE(struct task_handler);
};

extern void task_init(void);

extern void task_add(struct task_handler *task);

/**
 * @brief Remove a registered task.
 * 
 * @param task Pointer to a the callback.
 */
extern bool task_remove(struct task_handler *task);
	
#endif //TASK_H
