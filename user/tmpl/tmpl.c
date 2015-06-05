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
	
	//Create a context.
	context = (struct tmpl_context *)db_malloc(sizeof(struct tmpl_context), "context init_tmpl");
	context->template = template;
	context->tmpl_size = os_strlen(template);
	context->vars = NULL;
	
	debug(" Created context %p template size %d.\n", context, context->tmpl_size);
	
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
									void *value, enum tmpl_var_type type)
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
	var->type = type;
	
	//Get the size of the string representation.
	if (var->type == TMPL_INT)
	{
		var->value.v_int = *(int *)(value);
		var->char_size = digits(var->value.v_int);
		var->str = db_malloc(var->char_size, "var->str tmpl_add_var");
		itoa(var->value.v_int, var->str, 10);
	}
	else if (var->type == TMPL_STRING)
	{
		var->value.v_str = (char *)(value);
		var->char_size = os_strlen(var->value.v_str);
		var->str = var->value.v_str;
	}
	else if (var->type == TMPL_FLOAT)
	{
		var->value.v_flt = *(float *)(value);
		var->char_size = digits_f(var->value.v_flt, TMPL_FRAG_DIGITS);
		var->str = db_malloc(var->char_size, "var->str tmpl_add_var");
		ftoa(var->value.v_flt, var->str, TMPL_FRAG_DIGITS);
	}
	else
	{
		error("Wrong value type adding template variable.\n");
		db_free(var);
		return;
	}
	debug(" Value: %s.\n", var->str);
	
	if (context->vars)
	{
		DL_LIST_ADD_END(var, context->vars);
	}
	else
	{
		context->vars = var;
	}
}

/**
 * @brief Get a template variable by name.
 * 
 * @param context Context in which to find the variable.
 * @param name The name of the variable to get.
 * @return A pointer to a #tmpl_var struct of NULL.
 */
 static struct tmpl_var ICACHE_FLASH_ATTR *get_var(struct tmpl_context *context, char *name)
 {
	 
	 struct tmpl_var *var = context->vars;
	 
	 debug ("Looking for \"%s\" in the variables of context %p...", name, context);	
	 while (var)
	 {
		 if (os_strcmp(var->name, name) == 0)
		 {
			 debug(" Found %p.\n", var);
			 return(var);
		 }
		 var = var->next;
	 }
	 debug(" Not found.\n");
	 return(NULL);
 }	  

/**
 * @brief Get the size of a token.
 * 
 * @param token Pointer to the start of the token.
 * @return Size of the token.
 */
static size_t ICACHE_FLASH_ATTR get_token_size(char *token)
{
	size_t size = 0;
	
	while (*token != TMPL_TOKEN_ID[2])
	{
		//If string ends there is an error.
		if (*token == '\0')
		{
			error("End of template while parsing token.\n");
			return(0);
		}
		size++;
		token++;
	}
	return(size);
}

/**
 * @brief Get all tokens in a template into a nice structure,.
 * 
 * @param context The context to convert.
 * @return A pointer to a struct #tmpl_tokens, with all tokens in it.
 */
static struct tmpl_tokens ICACHE_FLASH_ATTR *get_tokens(struct tmpl_context *context)
{
	struct tmpl_tokens *tokens;
	struct tmpl_token *token;
	struct tmpl_var *var;
	char *current_var;
	size_t offset = 0;
	
	debug("Finding tokens in template %p.\n", context);
	
	//Get mem.
	tokens = db_malloc(sizeof(struct tmpl_tokens), "tokens get_tokens");

	//Count tokens.
	//Find every instance of the first character in the token.
	debug("Counting tokens.");
	current_var = os_strchr(context->template, TMPL_TOKEN_ID[0]);
	while (current_var != NULL)
	{
		//If the second character matches, this is a token
		if (*(current_var + 1 ) == TMPL_TOKEN_ID[1])
		{
			current_var = os_strchr(current_var, TMPL_TOKEN_ID[2]);
			tokens->n_tokens++;
			debug(".");
		}
		current_var = os_strchr(current_var, TMPL_TOKEN_ID[0]);
	}
	debug("%d\n", tokens->n_tokens);
	
	//Get mem for tokens.
	tokens->tokens = db_malloc(sizeof(struct tmpl_token) * tokens->n_tokens, "tokens tmpl_apply");
	token = tokens->tokens;
	//Find every instance of the first character in the token.
	current_var = os_strchr(context->template, TMPL_TOKEN_ID[0]);
	while (current_var != NULL)
	{
		
		//If the second character matches, this is a token
		if (*(current_var + 1 ) == TMPL_TOKEN_ID[1])
		{
			//Gather infos on the token.
			token->start = (current_var - context->template) - offset;

			token->size = get_token_size(current_var);
			//Add space for ${}
			token->end = token->start + token->size + 3;
			//Save offset for next token.
			offset = token->end;
			
			token->token = db_malloc(token->size, "token->token tmpl_apply");
			os_memcpy(token->token, current_var + 2, token->size - 2);
			token->token[token->size] = '\0';
			
			debug(" Found token \"%s\" at %d size %d.\n", token->token, token->start, token->size);
			
			//Add to the total characters count of all tokens.
			tokens->token_chars += token->size;
			
			var = get_var(context, token->token);
			if (var)
			{
				tokens->var_chars += var->char_size;
				token->value = var->str;
			}
			else
			{
				error("Could not find template variable \"%s\".\n", token->token);
			}
			
			current_var = os_strchr(½	token->end, TMPL_TOKEN_ID[0]);
			token++;
		}
	}
	debug("Total characters in tokens %d.\n", tokens->token_chars);
	debug("Total characters in substituted variable values %d.\n", tokens->var_chars);
	
	return(tokens);
}
		
/**
 * @brief Convert template to text.
 * 
 * @param context The context to convert.
 * @return The final result.
 */
char ICACHE_FLASH_ATTR *tmpl_apply(struct tmpl_context *context)
{
	struct tmpl_tokens *tokens;
	struct tmpl_token *token;
	size_t n_tokens = 0;
	struct tmpl_var *var;
	char *current_var;
	char *result;
	size_t i;
	size_t result_offset = 0;
	size_t tmpl_offset = 0;
	size_t block_size = 0;
	
	debug("Converting template %p.\n", context);

	tokens = get_tokens(context);
	context->tokens = tokens;
	
	//Replace the tokens with their value.
	if (!tokens->n_tokens)
	{
		//Nothing to do.
		result = tmpl_context->template;
	}
	else
	{
		result = (char *)db_malloc(context->tmpl_size - tokens->token_chars + tokens->var_chars,
							       "result tmpl_apply");

		while (i < context->tokens->n_tokens)
		{
			//Copy the string up until the token.
			block_size = tokens->tokens[i].start;
			//Do it.
			os_memcpy(result + result_offset, context->template + tmpl_offset, block_size);
			//Forward to the end of the block we've just copied.
			result_offset = block_size;
			//And the template to the end of the token.
			tmpl_offset = tokens->tokens[i].end;
			//Copy the token value.
			block_size = tokens->tokens[i].char_size;
			//Do it.
			os_memcpy(result + result_offset, tokens->tokens[i].value, block_size);
			//Add to the position of the result.
			result_offset += block_size;
		}
		//Copy the end.
		block_size = context->tmpl_size - tokens->tokens[i].end;
		memcpy(result + result_offset, context->template, block_size);
	}
							   
	return(result);
}

/**
 * @brief Free resources used by a context.
 * 
 * @param context Pointer to the context to free.
 */
void ICACHE_FLASH_ATTR tmpl_destroy(struct tmpl_context *context)
{
	struct tmpl_var *vars = context->vars;
	
	debug("Destroying template context %p.\n", context);
	while (vars)
	{
		context->vars = vars;
		db_free(context->vars);
		vars = vars->next;
	}
	db_free(context);
}
