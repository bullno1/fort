#include "internal.h"
#include <string.h>
#include <stdlib.h>
#include <bk/array.h>
#include <errno.h>

static fort_err_t
fort_parse_number(
	fort_string_ref_t string, fort_cell_type_t* type, fort_cell_t* value
)
{
	void* dot_ptr = memchr(string.ptr, '.', string.length);
	char* end_ptr;

	errno = 0;
	if(dot_ptr == NULL)
	{
		fort_int_t num = FORT_STR_TO_INT(string.ptr, &end_ptr, 0);
		*type = FORT_INTEGER;
		*value = (fort_cell_t){ .integer = num };
	}
	else
	{
		fort_real_t num = FORT_STR_TO_REAL(string.ptr, &end_ptr);
		*type = FORT_REAL;
		*value = (fort_cell_t){ .real = num };
	}

	int convert_error = errno == ERANGE || end_ptr != string.ptr + string.length;
	return convert_error ? FORT_ERR_TYPE : FORT_OK;
}

static fort_err_t
fort_interpret_token(fort_t* fort, const fort_token_t* token)
{
	const fort_word_t* word = fort_find_internal(fort, token->lexeme);
	if(word == NULL)
	{
		fort_cell_type_t type;
		fort_cell_t value;

		fort_err_t err = fort_parse_number(token->lexeme, &type, &value);
		if(err != FORT_OK) { return FORT_ERR_NOT_FOUND; }

		bk_array_push(fort->param_stack_types, type);
		bk_array_push(fort->param_stack_values, value);

		return FORT_OK;
	}
	else
	{
		return word->code(fort, word);
	}
}

static fort_err_t
fort_compile_token(fort_t* fort, const fort_token_t* token)
{
	(void)fort;
	(void)token;

	return FORT_ERR_NOT_FOUND;
}

static fort_err_t
fort_outer_interpret(fort_t* fort)
{
	for(;;)
	{
		fort_token_t token;
		fort_err_t err = fort_next_token(fort, &token);

		if(err == FORT_ERR_NOT_FOUND)
		{
			return FORT_OK;
		}
		else if(err != FORT_OK)
		{
			return err;
		}

		if(fort->state.interpreting)
		{
			if((err = fort_interpret_token(fort, &token)) != FORT_OK)
			{
				return err;
			}
		}
		else
		{
			if((err = fort_compile_token(fort, &token)) != FORT_OK)
			{
				return err;
			}
		}
	}
}

fort_err_t
fort_interpret(fort_t* fort, struct bk_file_s* in)
{
	fort_state_t state = fort->state;

	fort->state = (fort_state_t) {
		.input = in,
		.location = { .line = 1, .column = 0 },
		.interpreting = 1
	};

	fort_err_t err = fort_outer_interpret(fort);

	fort->state = state;
	return err;
}

fort_int_t
fort_is_interpreting(fort_t* fort)
{
	return (fort_int_t)fort->state.interpreting;
}
