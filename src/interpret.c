#include "internal.h"
#include <bk/array.h>
#include <bk/fs/mem.h>
#include <fort-utils.h>

static fort_err_t
fort_interpret_token(fort_t* fort, const fort_token_t* token)
{
	fort_word_t* word = fort_find_internal(fort, token->lexeme);
	if(word == NULL)
	{
		fort_cell_t value;

		fort_err_t err = fort_parse_number(token->lexeme, &value);
		if(err != FORT_OK) { return FORT_ERR_NOT_FOUND; }

		return fort_push(fort, value);
	}
	else
	{
		FORT_ASSERT(!word->compile_only, FORT_ERR_COMPILE_ONLY);

		return word->code(fort, word);
	}
}

static fort_err_t
fort_compile_token(fort_t* fort, const fort_token_t* token)
{
	fort_word_t* word = fort_find_internal(fort, token->lexeme);
	if(word == NULL)
	{
		fort_cell_t value;

		fort_err_t err = fort_parse_number(token->lexeme, &value);
		FORT_ASSERT(err == FORT_OK, FORT_ERR_NOT_FOUND);

		return fort_compile_internal(fort, value);
	}
	else
	{
		if(word->immediate)
		{
			return word->code(fort, word);
		}
		else
		{
			return fort_compile_internal(fort, (fort_cell_t){
				.type = FORT_XT,
				.data = { .ref = (fort_word_t*)word }
			});
		}
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
			FORT_ENSURE(fort_interpret_token(fort, &token));
		}
		else
		{
			FORT_ENSURE(fort_compile_token(fort, &token));
		}
	}
}

fort_err_t
fort_interpret(fort_t* fort, struct bk_file_s* in, fort_string_ref_t filename)
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

fort_err_t
fort_interpret_string(fort_t* fort, fort_string_ref_t str, fort_string_ref_t filename)
{
	bk_mem_file_t mem_file;
	bk_file_t* file = bk_mem_fs_wrap_fixed(&mem_file, (char*)str.ptr, str.length);
	return fort_interpret(fort, file, filename);
}

fort_int_t
fort_is_interpreting(fort_t* fort)
{
	return (fort_int_t)fort->state.interpreting;
}
