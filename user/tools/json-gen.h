/**
 * @file json-gen.h
 *
 * @brief Routines for generating JSON strings.
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
#ifndef JSON_GEN_H
#define JSON_GEN_H

#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"

/**
 * @brief Create a JSON array of integers.
 * 
 * @param values Pointer to an array of long ints.
 * @param entries Number of entries in the array.
 * @return Pointer to the JSON array.
 */
static char *json_create_int_array(long *values, size_t entries)
/**
 * @brief Create a JSON array of strings.
 * 
 * @param values Array of strings.
 * @param entries Number of strings in array.
 * @return The JSON array.
 */
extern char *json_add_to_object(char *json_string, char *member);
/**
 * @brief Create a member for an object.
 * 
 * @param string The string part of the member pair.
 * @param value The value part of the member pair.
 * @param Adds double quotes to value if TRUE.
 * @return A pointer to the string representation of the JSON member.
 */
extern char *json_create_member(char *string, char *value, bool quotes);
/**
 * @brief Add an element to a JSON object.
 * 
 * @param json_string Pointer to previous string representation of the object, or NULL to create.
 * @param member Pointer to the string representation of the member to add.
 * @return Pointer to the string representation of the of the new JSON object.
 */
extern char *json_create_string_array(char **values, size_t entries);

#endif //JSON_GEN_H
