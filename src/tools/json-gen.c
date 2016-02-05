
/**
 * @file json-gen.c
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
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"

char *json_create_pair(char *string, char *value, bool quotes)
{
	char *ret;
	char *offset;
	size_t size;
	size_t value_size;
	size_t string_size;
	
	debug("Creating JSON member from %s, %s.\n", string, value);
	if ((!string) || (!value))
	{
		warn(" Emtpy parameter passed.\n");
		return(NULL);
	}
	value_size = strlen(value); 
	string_size = strlen(string);
	//Size of string + "", :, and \0.
	size = string_size + value_size + 4;
	//Add memory for possible double quotes.
	if (quotes)
	{
		size += 2;
	}
	ret = db_malloc(size, "json_create_member ret");
	offset = ret;
	*offset++ = '\"';
	os_memcpy(offset, string, string_size);
	offset += string_size;
	os_memcpy(offset, "\":", 2);
	offset += 2;
	if (quotes)
	{
		*offset++ = '\"';
	}
	os_memcpy(offset, value, value_size);
	offset += value_size;
	if (quotes)
	{
		*offset++ = '\"';
	}
	*offset++ = '\0';
	return(ret);
}

char *json_add_to_type(char *json_string, char *element, char *type)
{
	size_t element_size;
	size_t object_size;
	char *element_pos;
	char *ret = NULL;
	
	debug("Adding %s to JSON container %s (type %s).\n", element, json_string, type);
	if (!element)
	{
		warn("Nothing to add.\n");
		return(json_string);
	}
	element_size = os_strlen(element);
	if (!json_string)
	{
		debug(" New object.\n");
		//Reserve memory add space for {, }, and \0. 
		object_size = element_size + 3;
		ret = db_malloc(object_size, "json_add_to_object ret");
		ret[0] = type[0];
		element_pos = ret + 1;
	}
	else
	{
		debug("Existing object.\n");
		size_t old_object_size = strlen(json_string);
		//Reserve memory add space for ",".
		object_size = old_object_size + element_size + 1;
		ret = db_realloc(json_string, object_size,
						 "json_add_to_object ret");
		//Point to the ending } of the old object.
		element_pos = ret + old_object_size - 1;
		//Add ,
		*element_pos++ = ',';
	}
	//Add member.
	memcpy(element_pos, element, element_size);
	//Add ending } and \0.
	element_pos += element_size;
	*element_pos++ = type[1];
	*element_pos = '\0';

	return(ret);
}
