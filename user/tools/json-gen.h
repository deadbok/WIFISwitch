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

#define JSON_TYPE_ARRAY "[]"
#define JSON_TYPE_OBJECT "{}"

/**
 * @brief Create a pair for an object.
 * 
 * @param string The string part of the pair.
 * @param value The value part of the pair.
 * @param Adds double quotes to value if TRUE.
 * @return A pointer to the string representation of the JSON member.
 */
extern char *json_create_pair(char *string, char *value, bool quotes);
/**
 * @brief Add an element to a JSON array.
 * 
 * @param json_string Pointer to previous string representation of the object, or NULL to create.
 * @param member Pointer to the string representation of the element to add.
 * @return Pointer to the string representation of the of the new JSON object.
 */
#define json_add_to_array(json_string, element) json_add_to_type(json_string, element, JSON_TYPE_ARRAY)
/**
 * @brief Add an member to a JSON object.
 * 
 * @param json_string Pointer to previous string representation of the object, or NULL to create.
 * @param member Pointer to the string representation of the member to add.
 * @return Pointer to the string representation of the of the new JSON object.
 */
#define json_add_to_object(json_string, member) json_add_to_type(json_string, member, JSON_TYPE_OBJECT)
/**
 * @brief Add an member/element to a JSON object/array.
 * 
 * @param json_string Pointer to previous string representation of the object, or NULL to create.
 * @param element Pointer to the string representation of the element/member to add.
 * @return Pointer to the string representation of the of the new JSON object.
 */
extern char *json_add_to_type(char *json_string, char *element, char *type);

#endif //JSON_GEN_H
