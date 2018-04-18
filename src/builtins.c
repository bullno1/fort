#include "internal.h"
#include <bk/assert.h>
#include <bk/array.h>
#include <fort/utils.h>

static int
fort_is_numeric(fort_cell_type_t type)
{
	return type == FORT_REAL || type == FORT_INTEGER;
}

static void
fort_promote_int_to_real(fort_cell_t* cell)
{
	cell->type = FORT_REAL;
	cell->data.real = (fort_real_t)cell->data.integer;
}

static fort_err_t
fort_numeric_2pop(fort_t* fort, fort_cell_t* lhs, fort_cell_t* rhs)
{
	FORT_ENSURE(fort_peek(fort, 0, rhs));
	FORT_ENSURE(fort_peek(fort, 1, lhs));

	FORT_ASSERT(
		fort_is_numeric(lhs->type) && fort_is_numeric(rhs->type),
		FORT_ERR_TYPE
	);

	if(lhs->type != FORT_INTEGER || rhs->type != FORT_INTEGER)
	{
		if(lhs->type == FORT_INTEGER) { fort_promote_int_to_real(lhs); }
		if(rhs->type == FORT_INTEGER) { fort_promote_int_to_real(rhs); }
	}

	return fort_ndrop(fort, 2);
}

static fort_err_t
fort_add(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_cell_t lhs, rhs;

	FORT_ENSURE(fort_numeric_2pop(fort, &lhs, &rhs));

	if(lhs.type == FORT_REAL)
	{
		return fort_push_real(fort, lhs.data.real + rhs.data.real);
	}
	else
	{
		return fort_push_integer(fort, lhs.data.integer + rhs.data.integer);
	}
}

static fort_err_t
fort_end_colon(fort_t* fort, fort_word_t* word)
{
	(void)fort;
	word->code = &fort_exec_colon;
	return FORT_OK;
}

static fort_err_t
fort_colon(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_token_t name;
	fort_err_t err = fort_next_token(fort, &name);
	FORT_ASSERT(err != FORT_ERR_NOT_FOUND, FORT_ERR_SYNTAX);
	FORT_ASSERT(err == FORT_OK, err);

	fort->compiling = 1;

	return fort_begin_word(fort, name.lexeme, fort_end_colon);
}

static fort_err_t
fort_last_word(fort_t* fort, fort_word_t** wordp)
{
	*wordp = fort->current_word != NULL ? fort->current_word : fort->last_word;
	return *wordp != NULL ? FORT_OK : FORT_ERR_NOT_FOUND;
}

static fort_err_t
fort_def_end(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ASSERT(fort->current_word != NULL, FORT_ERR_NOT_FOUND);
	FORT_ENSURE(fort->current_word->code(fort, fort->current_word));

	return fort_end_word(fort);
}

static fort_err_t
fort_bracket_open(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort->compiling = 0;
	return FORT_OK;
}

static fort_err_t
fort_bracket_close(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort->compiling = 1;
	return FORT_OK;
}

static fort_err_t
fort_immediate(fort_t* fort, fort_word_t* word)
{
	(void)word;
	fort_word_t* last_word;
	FORT_ENSURE(fort_last_word(fort, &last_word));

	last_word->immediate = 1;

	return FORT_OK;
}

static fort_err_t
fort_compile_only(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_word_t* last_word;
	FORT_ENSURE(fort_last_word(fort, &last_word));
	last_word->compile_only = 1;

	return FORT_OK;
}

static fort_err_t
fort_tick(fort_t* fort, fort_word_t* word)
{
	fort_token_t token;
	fort_err_t err = fort_next_token(fort, &token);
	FORT_ASSERT(err != FORT_ERR_NOT_FOUND, FORT_ERR_SYNTAX);
	FORT_ASSERT(err == FORT_OK, err);

	fort_word_t* ticked_word = fort_find_internal(fort->ctx, token.lexeme);
	if(ticked_word == NULL)
	{
		fort_cell_t value;
		if(fort_parse_number(token.lexeme, &value) != FORT_OK)
		{
			return FORT_ERR_NOT_FOUND;
		}
		else
		{
			return fort_push(fort, value);
		}
	}
	else
	{
		return fort_push(fort, (fort_cell_t){
			.type = word->data[0].data.integer,
			.data = { .ref = ticked_word }
		});
	}
}

static fort_err_t
fort_scan_until_char(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t end_char;
	FORT_ENSURE(fort_pop_integer(fort, &end_char));

	// TODO: record start pos

	bk_array_clear(fort->scan_buf);
	char ch;
	for(;;)
	{
		fort_err_t err = fort_next_char(fort, &ch);

		if(err == FORT_ERR_NOT_FOUND)
		{
			return FORT_ERR_SYNTAX;
		}
		else if(err != FORT_OK)
		{
			return err;
		}

		if(ch == end_char)
		{
			fort_string_ref_t ref = {
				.length = bk_array_len(fort->scan_buf),
				.ptr = fort->scan_buf
			};

			return fort_push_string(fort, ref);
		}
		else
		{
			bk_array_push(fort->scan_buf, ch);
		}
	}
}

static fort_err_t
fort_get_next_char(fort_t* fort, fort_word_t* word)
{
	(void)word;

	char ch;
	FORT_ENSURE(fort_next_char(fort, &ch));

	return fort_push_integer(fort, ch);
}

static fort_err_t
fort_compile(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_cell_t cell;
	FORT_ENSURE(fort_peek(fort, 0, &cell));
	FORT_ENSURE(fort_compile_internal(fort, cell));

	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_return(fort_t* fort, fort_word_t* word)
{
	(void)word;

	size_t return_stack_depth = bk_array_len(fort->return_stack);
	FORT_ASSERT(return_stack_depth > 0, FORT_ERR_UNDERFLOW);

	fort->current_frame = fort->return_stack[return_stack_depth - 1];
	bk_array_resize(fort->return_stack, return_stack_depth - 1);

	return word->data[0].data.integer;
}

static fort_err_t
fort_make_free_word(
	fort_t* fort,
	fort_string_ref_t name,
	fort_native_fn_t code,
	unsigned immediate, unsigned compile_only
)
{
	FORT_ENSURE(fort_begin_word(fort, name, code));

	fort->current_word->immediate = immediate;
	fort->current_word->compile_only = compile_only;

	return fort_end_word(fort);
}

static fort_err_t
fort_make_closed_word(
	fort_t* fort,
	fort_string_ref_t name,
	fort_native_fn_t code,
	fort_int_t data,
	unsigned immediate, unsigned compile_only
)
{
	FORT_ENSURE(fort_begin_word(fort, name, code));

	FORT_ENSURE(fort_compile_internal(fort, (fort_cell_t){
		.type = FORT_INTEGER,
		.data = { .integer = data }
	}));

	fort->current_word->immediate = immediate;
	fort->current_word->compile_only = compile_only;

	return fort_end_word(fort);
}

fort_err_t
fort_load_builtins(fort_t* fort)
{
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("+"), &fort_add, 0, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("["), &fort_bracket_open, 1, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("]"), &fort_bracket_close, 0, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("immediate"), &fort_immediate, 1, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("compile-only"), &fort_compile_only, 1, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("compile"), &fort_compile, 0, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF(":"), &fort_colon, 0, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("def-end"), &fort_def_end, 0, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("scan-until-char"), &fort_scan_until_char, 0, 0));
	FORT_ENSURE(fort_make_free_word(fort, FORT_STRING_REF("next-char"), &fort_get_next_char, 0, 0));
	FORT_ENSURE(fort_make_closed_word(fort, FORT_STRING_REF("exit"), &fort_return, FORT_OK, 0, 0));
	FORT_ENSURE(fort_make_closed_word(fort, FORT_STRING_REF("switch"), &fort_return, FORT_SWITCH, 0, 0));
	FORT_ENSURE(fort_make_closed_word(fort, FORT_STRING_REF("'"), &fort_tick, FORT_XT, 0, 0));
	FORT_ENSURE(fort_make_closed_word(fort, FORT_STRING_REF("[']"), &fort_tick, FORT_TICK, 1, 1));

	fort_string_ref_t core = FORT_STRING_REF(
		": ; immediate compile-only ['] exit [ compile ] compile def-end [ ' [ compile ] exit [ def-end\n"
		": return-to-native switch [ def-end\n"
		": c immediate next-char ;\n"
		": [compile] immediate compile-only compile ;\n"
		": \" c \" [compile] scan-until-char ;\n"
	);
	FORT_ENSURE(fort_interpret_string(fort, core, FORT_STRING_REF("<core>")));

	return FORT_OK;
}
