#include "internal.h"
#include <bk/array.h>
#include <errno.h>

static fort_err_t
fort_interpret_token(fort_t* fort, const fort_token_t* token)
{
	const fort_word_t* word = fort_find_internal(fort, token->lexeme);
	if(word == NULL)
	{
		fort_cell_t value;

		fort_err_t err = fort_parse_number(token->lexeme, &value);
		if(err != FORT_OK) { return FORT_ERR_NOT_FOUND; }

		return fort_push(fort, value);
	}
	else
	{
		return word->code(fort, word);
	}
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
fort_interpret(fort_t* fort, struct bk_file_s* in, const char* filename)
{
	(void)filename;
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
