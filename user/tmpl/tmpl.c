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
	
	debug(" Created context %p.\n", context);
	
	return(context);
}

/**
 * @brief Apply template to a string.
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
