
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

static char *json_create_array(long *values, size_t entries, bool quotes)
{
	size_t i;
	size_t total_length = 0;
	size_t *lengths = db_malloc(sizeof(size_t) * entries, "lengths json_create_array");
	char *ret, *ptr;
	
	//'['
	total_length++;
	for (i = 0; i < entries; i++)
	{
		lengths[i] = digits(values[i]);
		total_length += lengths[i];
		//","
		if (i != (entries -1))
		{
			total_length++;
		}
	}
	//']'
	total_length++;
	
	ret = (char *)db_malloc(total_length, "res json_create_array");
	ptr = ret;
	
	*ptr++ = '[';
	for (i = 0; i < entries; i++)
	{
		itoa(values[i], ptr, 10);
		ptr += lengths[i] - 1;
		//","
		if (i != (entries -1))
		{
			*ptr++ = ',';
		}
	}
	*ptr++ = ']';
	*ptr = '\0';
	
	db_free(lengths);
	
	return(ret);
}

char *json_create_string_array(char **values, size_t entries)
{
	size_t i;
	size_t total_length = 0;
	size_t *lengths = db_malloc(sizeof(size_t) * entries, "lengths json_create_string_array");
	char *ret, *ptr;
	
	//Get the length of the JSON array.
	//'['
	total_length++;
	for (i = 0; i < entries; i++)
	{
		//'\"'
		total_length++;
		lengths[i] = os_strlen(values[i]);
		total_length += lengths[i];
		//"\","
		total_length++;
		if (i != (entries -1))
		{
			total_length++;
		}
	}
	//']'
	total_length++;
	
	ret = (char *)db_malloc(total_length, "res json_create_string_array");
	ptr = ret;
	
	//Build the actual JSON array as a string.
	*ptr++ = '[';
	for (i = 0; i < entries; i++)
	{
		*ptr++ = '\"';
		os_memcpy(ptr, values[i], lengths[i]);
		ptr += lengths[i];
		*ptr++ = '\"';
		if (i != (entries -1))
		{
			*ptr++ = ',';
		}
	}
	*ptr++ = ']';
	*ptr = '\0';
	
	db_free(lengths);
	
	return(ret);
}

char *json_create_member(char *string, char *value, bool quotes)
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

char *json_add_to_object(char *json_string, char *member)
{
	size_t member_size;
	size_t object_size;
	char *member_pos;
	char *ret = NULL;
	
	debug("Adding member %s to JSON object %s.\n", member, json_string);
	if (!member)
	{
		warn("Nothing to add.\n");
		return(json_string);
	}
	member_size = os_strlen(member);
	if (!json_string)
	{
		debug(" New object.\n");
		//Reserve memory add space for {, }, and \0. 
		object_size = member_size + 3;
		ret = db_malloc(object_size, "json_add_to_object ret");
		ret[0] = '{';
		member_pos = ret + 1;
	}
	else
	{
		debug("Existing object.\n");
		size_t old_object_size = strlen(json_string);
		//Reserve memory add space for ",".
		object_size = old_object_size + member_size + 1;
		ret = db_realloc(json_string, object_size,
						 "json_add_to_object ret");
		//Point to the ending } of the old object.
		member_pos = ret + old_object_size - 1;
		//Add ,
		*member_pos++ = ',';
	}
	//Add member.
	memcpy(member_pos, member, member_size);
	//Add ending } and \0.
	memcpy(member_pos + member_size, "}\0", 2);

	
	return(ret);
}

