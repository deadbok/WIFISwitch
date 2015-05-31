/** @file tmpl.c
 *
 * @brief Simple template enginde.
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
#include "tools/missing_dec.h"
#include "c_types.h"
#include "osapi.h"
#include "user_config.h"
#include "tools/strxtra.h"
#include "tmpl.h"

/**
 * @brief Initialise template engine.
 * 
 * @param template A pointer to the text of template to work with.
 * @return A pointer to the created template context.
 */
struct tmpl_context ICACHE_FLASH_ATTR *init_tmpl(char *template)
{
	struct tmpl_context *context;
	
	debug("Initialising template engine.\n");
	
	context = (struct tmpl_context *)db_malloc(sizeof(struct tmpl_context), "context init_tmpl");
	context->template = template;
	context->tmpl_size = os_strlen(template);
	context->vars = NULL;
	
	debug(" Created context %p.\n", context);
	
	return(context);
}

/**
 * @brief Add a template variable.
 * 
 * @param context The template context to add the variable to.
 * @param name The name of the variable.
 * @param value The value of the variable.
 * @param type Type of the variable, valid values are in enum #tmpl_var_type.
 */
void ICACHE_FLASH_ATTR tmpl_add_var(struct tmpl_context *context, char *name,
									union tmpl_val_type value, enum tmpl_var_type type)
{
	struct tmpl_var *var;
	debug("Adding variable %s to template context %p.\n", name, context);
	
	if (!context)
	{
		return;
	}
	
	//Get mem and fill in the data.
	var = db_malloc(sizeof(struct tmpl_var), "var tmpl_add_var");
	var->name = name;
	var->value = value;
	var->type = type;
	
	//Get the size of the string representation.
	if (var->type == TMPL_INT)
	{
		var->char_size = digits(var->value.v_int);
	}
	else if (var->type == TMPL_STRING)
	{
		var->char_size = os_strlen(var->value.v_str);
	}
	else if (var->type == TMPL_FLOAT)
	{
		var->char_size = digits_f(var->value.v_flt, 2);
	}
	else
	{
		error("Wrong value type adding template variable.\n");
		db_free(var);
		return;
	}
	
	DL_LIST_ADD_END(var, context->vars);
}
/**
 * @brief Convert template to text.
 * 
 * @param context The context to convert.
 */
char ICACHE_FLASH_ATTR *tmpl_apply(struct tmpl_context *context)
{
	return(context->template);
}

/**
 * @brief Free resources used by a context.
 * 
 * @param context Pointer to the context to free.
 */
void ICACHE_FLASH_ATTR tmpl_destroy(struct tmpl_context *context)
{
	debug("Destroying template context %p.\n", context);
	db_free(context);
}
