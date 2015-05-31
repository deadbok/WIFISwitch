/** @file tmpl.h
 *
 * @brief Simple template enginde
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
#ifndef TMPL_H
#define TMPL_H

#include "tools/dl_list.h"
/**
 * @brief Variable types known by the template engine.
 */
enum tmpl_var_type
{
	/**
	 * @brief Integer.
	 */
	TMPL_INT,
	/**
	 * @brief Floating point number.
	 */
	TMPL_FLOAT,
	/**
	 * @brief String.
	 */
	TMPL_STRING
};

/**
 * @brief Union holds all the variable types of a template variable.
 */
union tmpl_val_type
{
	/**
	 * @brief String representation.
	 */
	char *v_str;
	/**
	 * @brief Interger representation.
	 */
	long v_int;
	/**
	 * @brief Float representation.
	 */
	float v_flt;
};

/**
 * @brief Structure for a template variable.
 */
struct tmpl_var
{
	/**
	 * @brief Name of the variable.
	 */
	char *name;
	/**
	 * @brief Pointer to the value.
	 */
	 
	union tmpl_val_type value;
	/**
	 * @brief Variable type.
	 */
	enum tmpl_var_type type;
	/**
	 * @brief Size of a string representation of the variable.
	 */
	size_t char_size;
	/**
	 * @brief Make it a linked list.
	 */
	DL_LIST_CREATE(struct tmpl_var);
};
/**
 * @brief Template context, that stores information about a template.
 */
struct tmpl_context
{
	/**
	 * @brief The actual template.
	 */
	char *template;
	/**
	 * @brief Number of characters in the template.
	 */
	size_t tmpl_size;
	/**
	 * @brief Linked list of template variables.
	 */
	struct tmpl_var *vars;
};

extern struct tmpl_context *init_tmpl(char *template);
extern void tmpl_add_var(struct tmpl_context *context, char *name,
									union tmpl_val_type value, 
									enum tmpl_var_type type);
extern char *tmpl_apply(struct tmpl_context *context);
extern void tmpl_destroy(struct tmpl_context *context);

#endif //TMPL_H
