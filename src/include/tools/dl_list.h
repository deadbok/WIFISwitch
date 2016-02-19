/** @file dl_list.h
 *
 * @brief Doubly-linked list helpers.
 * 
 * Macros to make handling of doubly-linked lists easier.
 * 
 * @todo All these macros are bound to be replaced, they seemed like 
 *       good idea at the time, but the side effects are treacherous.
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

/**
 * @brief Insert pointers for a doubly-linked list.
 * 
 * Use this to add doubly-linked list capabilities to a struct, by adding the
 * `prev` and `next` pointers.
 */
#define DL_LIST_CREATE(TYPE)    TYPE *next;\
                                TYPE *prev

/**
 * @brief Go to the last node.
 * 
 * *Address of NODE is modified.*
 * 
 * @param NODE Pointer to a linked list. 
 * @return NODE will point at the last node.
 */
#define DL_LIST_END(NODE)   while(NODE->next != NULL)\
                                NODE = NODE->next
/**
 * @brief Unlink a node from the list.
 * 
 * If LIST is pointing at the same address as node, LIST will be
 * advanced to the next node in the list.
 * 
 * *Address of LIST might be modified. Links in LIST are modified.*
 * 
 * @param NODE Node to unlink.
 * @param LIST List to unlink node from.
 */
#define DL_LIST_UNLINK(NODE, LIST)  if (LIST == NODE)\
                                        LIST = NODE->next;\
                                    if (NODE->prev != NULL)\
                                        NODE->prev->next = NODE->next;\
                                    if (NODE->next != NULL)\
                                        NODE->next->prev = NODE->prev
/**
 * @brief Insert a node into a list.
 * 
 * The macro does not check if PREV and NEXT has sane addresses, except
 * for NULL. The macro will also fail if NODE is NULL.
 * 
 * *Links in both NODE, PREV, and NEXT are modified.*
 * 
 * @param NODE Node to insert.
 * @param PREV Node that should come before NODE in the modified list.
 * @param NEXT Node that should come after NODE in the modified list.
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
 * The macro will fail if NODE or LIST is NULL.
 * 
 * *Address of LIST might be modified. Links in LIST and NODE
 * are modified.*
 * 
 * @param NODE Node to add.
 * @param LIST List to add the node to.
 */
#define DL_LIST_ADD_END(NODE, LIST) while(LIST->next != NULL)\
                                    {\
                                        LIST = LIST->next;\
                                    }\
                                    NODE->prev = LIST;\
                                    NODE->next = NULL;\
                                    LIST->next = NODE

#endif
