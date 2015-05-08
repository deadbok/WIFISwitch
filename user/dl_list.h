/** @file dl_list.h
 *
 * @brief Doubly-linked list helpers.
 * 
 * Macros to make handling of doubly-linked lists easier.
 * **Every macro should be expected to modify its parameters.**
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
#ifndef DL_LIST_H
#define DL_LIST_H

/* Wrapping stuf in do...while(0), makes it possible to invoke the macro like a
 * function call ending with semicolon (;).
 * Idea from here: [https://gcc.gnu.org/onlinedocs/cpp/Swallowing-the-Semicolon.html
 * Didn't use it, but it still clever ;-).
 */

/**
 * @brief Insert pointers for a doubly-linked list.
 * 
 * Use this to add doubly-linked list capabilities to a struct, by adding the
 * `prev` and `next` pointers.
 */
#define DL_LIST_CREATE(TYPE)    TYPE *next;\
                                TYPE *prev

/**
 * @brief Go to the end node.
 */
#define DL_LIST_END(NODE)   while(NODE->next != NULL)\
                                NODE = NODE->next
/**
 * @brief Unlink a node from the list.
 */
#define DL_LIST_UNLINK(NODE)    if (NODE->prev != NULL)\
                                    NODE->prev->next = NODE->next;\
                                if (NODE->next != NULL)\
                                    NODE->next->prev = NODE->prev
/**
 * @brief Insert a node into a list.
 * 
 * Insert a @p NODE into the list, between @p PREV and @p NEXT. **Fails if 
 * @p NODE is `NULL`.**
 */
#define DL_LIST_INSERT(NODE, PREV, NEXT)    if (NEXT != NULL)\
                                               NEXT->prev = NODE;\
                                            if (PREV != NULL)\
                                                PREV->next = NODE;\
                                            NODE->next = NEXT;\
                                            NODE->prev = PREV
/**
 * @brief Add a node to the end of the list.
 * 
 * Insert @p NODE at the end of @p LIST. **Fails if any parameter is `NULL`.**
 */
#define DL_LIST_ADD_END(NODE, LIST) while(LIST->next != NULL)\
                                    {\
                                        LIST = LIST->next;\
                                    }\
                                    NODE->prev = LIST;\
                                    NODE->next = NULL;\
                                    LIST->next = NODE

#endif
