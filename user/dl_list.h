/* dl_list.h
 *
 * Double linked list helpers.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
 * 
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

//Wrapping stuf in do...while(0), makes it possible to invoke the macro like a
//function call ending with semicolon (;).
//Idea from here: https://gcc.gnu.org/onlinedocs/cpp/Swallowing-the-Semicolon.html

//Use this to add doubly linked list capabilities to a struct.
#define DL_LIST_CREATE(TYPE)    TYPE *next;\
                                TYPE *prev

//Go to the end node
#define DL_LIST_END(NODE)   while(NODE->next != NULL)\
                                NODE = NODE->next
//Unlink a node from hte list.
#define DL_LIST_UNLINK(NODE)    if (NODE->next != NULL)\
                                    NODE->prev->next = NODE->next;\
                                if (NODE->prev != NULL)\
                                    NODE->next->prev = NODE->prev
//Insert a node into a list.
#define DL_LIST_INSERT(NODE, PREV, NEXT)    NEXT->prev = NODE;\
                                            PREV->next = NODE;\
                                            NODE->next = NEXT;\
                                            NODE->prev = PREV
/* Add to the end of the list. 
 * *Do not use on an empty list.*
 * LIST pointer is changed
 */
#define DL_LIST_ADD_END(NODE, LIST) while(LIST->next != NULL)\
                                    {\
                                        LIST = LIST->next;\
                                    }\
                                    NODE->prev = LIST;\
                                    NODE->next = NULL;\
                                    LIST->next = NODE

#endif
