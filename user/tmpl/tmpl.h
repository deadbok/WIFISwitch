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
 * @brief Fractional sigits used when converting a float to a string.
 */
#define TMPL_FRAG_DIGITS 2

/**
 * @brief Structure too keep track a token while parsing.
 */
struct tmpl_token
{
	/**
	 * @brief Offset of the token start.
	 */
	size_t start;
	/**
	 * @brief Offset of the token end.
	 */
	size_t end;
	/**
	 * @brief Size of the token.
	 */
	size_t size;
	/**
	 * @brief The token without indicator characters.
	 */
	char *token;
	/**
	 * @brief Replacement for the token.
	 */
	char *value; 
	/**
	 * @brief Make it a linked list.
	 */
	DL_LIST_CREATE(struct tmpl_token);
};

/**
 * @brief Structure too keep track of tokens while parsing.
 */
struct tmpl_tokens
{
	/**
	 * @brief Pointer to the tokens.
	 */
	struct tmpl_token *tokens;
	/**
	 * @brief Number of tokens.
	 */
	 unsigned short n_tokens;
	/**
	 * @brief Number of characters that all tokens takes up.
	 */
	size_t token_chars;
	/**
	 * @brief Number of characters the token substitutions take up.
	 */
	size_t var_chars;
};

/**
 * @brief Characters uses to identify a token in the template.
 */
#define TMPL_TOKEN_ID "${}"
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
	 * @brief Integer representation.
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
	 * @brief String version of the variable.
	 */
	char *str;
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
	/**
	 * @brief All tokens found in the template.
	 */
	struct tmpl_tokens tokens;
};

extern struct tmpl_context *init_tmpl(char *template);
extern void tmpl_add_var(struct tmpl_context *context, char *name,
									void *value, 
									enum tmpl_var_type type);
extern char *tmpl_apply(struct tmpl_context *context);
extern void tmpl_destroy(struct tmpl_context *context);

#endif //TMPL_H
