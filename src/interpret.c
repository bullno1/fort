#include "internal.h"
#include <bk/array.h>
#include <bk/fs/mem.h>
#include <fort/utils.h>

static fort_err_t
fort_interpret_token(fort_t* fort, const fort_token_t* token)
{
	fort_word_t* word = fort_find_internal(fort->ctx, token->lexeme);
	if(word == NULL)
	{
		fort_cell_t value;

		fort_err_t err = fort_parse_number(token->lexeme, &value);
		if(err != FORT_OK) { return FORT_ERR_NOT_FOUND; }

		return fort_push(fort, value);
	}
	else
	{
		FORT_ASSERT((word->flags & FORT_WORD_COMPILE_ONLY) == 0, FORT_ERR_COMPILE_ONLY);

		return word->code(fort, word);
	}
}

static char
fort_is_inlinable(fort_t* fort, fort_word_t* word)
{
	if(word->flags & FORT_WORD_NO_INLINE) { return 0; }
	if(word->code != fort_exec_colon) { return 0; }
	if(word->data == NULL) { return 0; }

	size_t length = bk_array_len(word->data);
	if(!(0 < length && length <= FORT_INLINE_THRESHOLD)) { return 0; }

	fort_cell_t* last_cell = &word->data[length - 1];
	if(!(last_cell->type == FORT_XT && last_cell->data.ref == fort->ctx->exit))
	{
		return 0;
	}

	return 1;
}

static fort_err_t
fort_compile_token(fort_t* fort, const fort_token_t* token)
{
	FORT_ASSERT(fort->current_word != NULL, FORT_ERR_NOT_FOUND);

	fort_word_t* word = fort_find_internal(fort->ctx, token->lexeme);
	if(word == NULL)
	{
		fort_cell_t value;

		fort_err_t err = fort_parse_number(token->lexeme, &value);
		FORT_ASSERT(err == FORT_OK, FORT_ERR_NOT_FOUND);

		return fort_push_word_data_internal(
			fort->ctx, fort->current_word, value
		);
	}
	else
	{
		if(word->flags & FORT_WORD_IMMEDIATE)
		{
			return word->code(fort, word);
		}
		else
		{
			fort_cell_t cell = {
				.type = FORT_XT,
				.data = { .ref = word }
			};

			// TODO: refactor to use fort_compile_internal
			if(fort_is_inlinable(fort, word))
			{
				size_t word_size = bk_array_len(word->data);
				for(size_t i = 0; i < word_size - 1; ++i)
				{
					FORT_ENSURE(fort_push_word_data_internal(fort->ctx, fort->current_word, word->data[i]));
				}

				return FORT_OK;
			}
			else
			{
				return fort_push_word_data_internal(fort->ctx, fort->current_word, cell);
			}
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

		if(fort->state == FORT_STATE_COMPILING)
		{
			FORT_ENSURE(fort_compile_token(fort, &token));
		}
		else
		{
			FORT_ENSURE(fort_interpret_token(fort, &token));
		}
	}
}

fort_err_t
fort_interpret(fort_t* fort, struct bk_file_s* in, fort_string_ref_t filename)
{
	(void)filename;

	// TODO: store all interpreter state in fort_interpreter_state_t structure
	fort_state_t state = fort->state;
	fort_input_state_t input_state = fort->input_state;
	fort_int_t exec_loop_level = fort->exec_loop_level;

	fort->state = FORT_STATE_INTERPRETING;
	fort->input_state = (fort_input_state_t) {
		.input = in,
		.location = { .line = 1, .column = 0 },
	};
	fort->exec_loop_level = 0;

	fort_err_t err = fort_outer_interpret(fort);

	fort->state = state;
	fort->input_state = input_state;
	fort->exec_loop_level = exec_loop_level;

	return err;
}

fort_err_t
fort_interpret_string(fort_t* fort, fort_string_ref_t str, fort_string_ref_t filename)
{
	bk_mem_file_t mem_file;
	bk_file_t* file = bk_mem_fs_wrap_fixed(&mem_file, (char*)str.ptr, str.length);
	return fort_interpret(fort, file, filename);
}
