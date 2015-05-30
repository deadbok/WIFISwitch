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

/**
 * @brief Template context, that stores information about a template.
 */
struct tmpl_context
{
	/**
	 * @brief The actual template.
	 */
	char *template;
	size_t tmpl_size;
};

extern struct tmpl_context *init_tmpl(char *template);
extern char *tmpl_apply(struct tmpl_context *context);
extern void tmpl_destroy(struct tmpl_context *context);

#endif //TMPL_H
