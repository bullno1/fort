#include "internal.h"
#include <bk/assert.h>
#include <bk/array.h>
#include <bk/printf.h>
#include <fort/utils.h>
#include "builtins_fs.h"

typedef union
{
	fort_int_t integer;
	fort_real_t real;
} fort_number_t;

static int
fort_is_numeric(fort_cell_type_t type)
{
	return type == FORT_REAL || type == FORT_INTEGER;
}

static fort_err_t
fort_numeric_2pop(
	fort_t* fort, fort_cell_type_t* type,
	fort_number_t* lhs, fort_number_t* rhs
)
{
	fort_cell_type_t lhs_type, rhs_type;
	FORT_ENSURE(fort_get_type(fort, 0, &rhs_type));
	FORT_ENSURE(fort_get_type(fort, 1, &lhs_type));

	FORT_ASSERT(fort_is_numeric(lhs_type) && fort_is_numeric(rhs_type), FORT_ERR_TYPE);

	if(lhs_type == FORT_INTEGER && rhs_type == FORT_INTEGER)
	{
		*type = FORT_INTEGER;
		FORT_ENSURE(fort_as_integer(fort, 0, &rhs->integer));
		FORT_ENSURE(fort_as_integer(fort, 1, &lhs->integer));
	}
	else
	{
		*type = FORT_REAL;
		FORT_ENSURE(fort_as_real(fort, 0, &rhs->real));
		FORT_ENSURE(fort_as_real(fort, 1, &lhs->real));
	}

	return fort_ndrop(fort, 2);
}

static fort_err_t
fort_add(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_cell_type_t type;
	fort_number_t lhs, rhs;

	FORT_ENSURE(fort_numeric_2pop(fort, &type, &lhs, &rhs));

	if(type == FORT_REAL)
	{
		return fort_push_real(fort, lhs.real + rhs.real);
	}
	else
	{
		return fort_push_integer(fort, lhs.integer + rhs.integer);
	}
}

static fort_err_t
fort_colon(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_token_t name;
	fort_err_t err = fort_next_token(fort, &name);
	FORT_ASSERT(err != FORT_ERR_NOT_FOUND, FORT_ERR_SYNTAX);
	FORT_ASSERT(err == FORT_OK, err);

	fort->state = FORT_STATE_COMPILING;

	return fort_begin_word(
		fort->ctx, name.lexeme, &fort_exec_colon, &fort->current_word
	);
}

static fort_err_t
fort_semicolon(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ASSERT(fort->current_word != NULL, FORT_ERR_SYNTAX);
	FORT_ENSURE(
		fort_push_word_data_internal(
			fort->ctx,
			fort->current_word,
			(fort_cell_t){
				.type = FORT_XT,
				.data = { .ref = fort->ctx->exit }
			}
		)
	);
	FORT_ENSURE(fort_end_word(fort->ctx, fort->current_word));
	fort->current_word = NULL;
	fort->state = FORT_STATE_INTERPRETING;

	return FORT_OK;
}

static fort_err_t
fort_state_set(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t state;
	FORT_ENSURE(fort_pop_integer(fort, &state));
	fort_set_state(fort, state);

	return FORT_OK;
}

static fort_err_t
fort_state_get(fort_t* fort, fort_word_t* word)
{
	(void)word;

	return fort_push_integer(fort, fort_get_state(fort));
}

static fort_err_t
fort_immediate(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ASSERT(fort->current_word != NULL, FORT_ERR_NOT_FOUND);
	fort->current_word->flags |= FORT_WORD_IMMEDIATE;

	return FORT_OK;
}

static fort_err_t
fort_compile_only(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ASSERT(fort->current_word != NULL, FORT_ERR_NOT_FOUND);
	fort->current_word->flags |= FORT_WORD_COMPILE_ONLY;

	return FORT_OK;
}

static fort_err_t
fort_tick(fort_t* fort, fort_word_t* word)
{
	(void)word;

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
			.type = FORT_XT,
			.data = { .ref = ticked_word }
		});
	}
}

static fort_err_t
fort_quote(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, 0, &cell));

	if(cell->type == FORT_XT) { cell->type = FORT_TICK; }

	return FORT_OK;
}

static fort_err_t
fort_scan_buf(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_string_ref_t ref = {
		.length = bk_array_len(fort->scan_buf),
		.ptr = fort->scan_buf
	};

	return fort_push_string(fort, ref);
}

static fort_err_t
fort_clear_scan_buf(fort_t* fort, fort_word_t* word)
{
	(void)word;

	bk_array_clear(fort->scan_buf);

	return FORT_OK;
}

static fort_err_t
fort_scan_until_char(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t end_char;
	FORT_ENSURE(fort_pop_integer(fort, &end_char));

	// TODO: record start pos

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
			return FORT_OK;
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

	fort_cell_t* cell;
	FORT_ASSERT(fort->current_word != NULL, FORT_ERR_SYNTAX);
	FORT_ENSURE(fort_stack_address(fort, 0, &cell));
	FORT_ENSURE(fort_push_word_data_internal(fort->ctx, fort->current_word, *cell));

	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_equal(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_cell_t *lhs, *rhs;
	FORT_ENSURE(fort_stack_address(fort, 0, &rhs));
	FORT_ENSURE(fort_stack_address(fort, 1, &lhs));
	FORT_ENSURE(fort_push_integer(fort, lhs->type == rhs->type && lhs->data.ref == rhs->data.ref));
	FORT_ENSURE(fort_move(fort, 0, 2));

	return fort_ndrop(fort, 2);
}

static void
fort_print_cell(fort_t* fort, fort_cell_t* cell)
{
	bk_printf(fort->config.output, "%s ", fort_cell_type_t_to_str(cell->type) + sizeof("FORT"));

	switch(cell->type)
	{
		case FORT_NULL:
			break;
		case FORT_REAL:
			bk_printf(fort->config.output, FORT_REAL_FMT, cell->data.real);
			break;
		case FORT_INTEGER:
			bk_printf(fort->config.output, FORT_INT_FMT, cell->data.integer);
			break;
		case FORT_STRING:
			if(cell->data.ref != NULL)
			{
				bk_printf(fort->config.output, "\"%s\"", ((fort_string_t*)cell->data.ref)->ptr);
			}
			else
			{
				bk_printf(fort->config.output, "\"\"");
			}
			break;
		case FORT_TICK:
		case FORT_XT:
			bk_printf(fort->config.output, "%p", cell->data.ref);
			break;
	}

	bk_printf(fort->config.output, "\n");
}

static fort_err_t
fort_print(fort_t* fort, fort_word_t* word)
{
	(void)word;
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, 0, &cell));

	fort_print_cell(fort, cell);

	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_print_stack(fort_t* fort, fort_word_t* word)
{
	(void)word;

	for(fort_int_t i = 0; i < fort_get_stack_size(fort); ++i)
	{
		fort_cell_t* cell;
		FORT_ENSURE(fort_stack_address(fort, -i - 1, &cell));
		fort_print_cell(fort, cell);
	}

	return FORT_OK;
}

static fort_err_t
fort_f_reset(fort_t* fort, fort_word_t* word)
{
	(void)word;

	return fort_reset(fort);
}

static fort_err_t
fort_f_pick(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t n;
	FORT_ENSURE(fort_pop_integer(fort, &n));
	return fort_pick(fort, n);
}

static fort_err_t
fort_f_roll(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t n;
	FORT_ENSURE(fort_pop_integer(fort, &n));
	return fort_roll(fort, n);
}

static fort_err_t
fort_f_ndrop(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t n;
	FORT_ENSURE(fort_pop_integer(fort, &n));
	return fort_ndrop(fort, n);
}

// Implement in C because this word is too common and `1 roll` is inefficient
static fort_err_t
fort_swap(fort_t* fort, fort_word_t* word)
{
	(void)word;
	fort_cell_t *a, *b;
	fort_cell_t tmp;

	FORT_ENSURE(fort_stack_address(fort, 0, &b));
	FORT_ENSURE(fort_stack_address(fort, 1, &a));
	tmp = *b;
	*b = *a;
	*a = tmp;

	return FORT_OK;
}

static fort_err_t
fort_current_word_get(fort_t* fort, fort_word_t* word)
{
	(void)word;

	return fort_push(fort, (fort_cell_t){
		.type = FORT_XT,
		.data = { .ref = fort->current_word }
	});
}

static fort_err_t
fort_current_word_set(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_word_t* top;
	FORT_ENSURE(fort_as_word(fort, 0, &top));

	fort->current_word = top;

	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_word_create(fort_t* fort, fort_word_t* word)
{
	(void)word;
	return fort_create_unnamed_word(fort);
}

static fort_err_t
fort_word_get_at(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t index;
	FORT_ENSURE(fort_pop_integer(fort, &index));

	FORT_ENSURE(fort_get_word_data(fort, NULL, index));
	FORT_ENSURE(fort_swap(fort, NULL));

	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_word_set_at(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t index;
	FORT_ENSURE(fort_swap(fort, NULL));
	FORT_ENSURE(fort_pop_integer(fort, &index));

	return fort_set_word_data(fort, NULL, index);
}

static fort_err_t
fort_word_push(fort_t* fort, fort_word_t* word)
{
	(void)word;

	return fort_push_word_data(fort, NULL);
}

static fort_err_t
fort_word_delete(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_int_t index;
	FORT_ENSURE(fort_pop_integer(fort, &index));

	return fort_delete_word_data(fort, NULL, index);
}

static fort_err_t
fort_word_length(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ENSURE(fort_get_word_length(fort, NULL));
	FORT_ENSURE(fort_swap(fort, NULL));
	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_word_name_get(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ENSURE(fort_get_word_name(fort, NULL));
	FORT_ENSURE(fort_swap(fort, NULL));
	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_word_name_set(fort_t* fort, fort_word_t* word)
{
	(void)word;
	return fort_set_word_name(fort, NULL);
}

static fort_err_t
fort_word_register(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ENSURE(fort_register_word(fort, NULL));
	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_word_flag_get(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ENSURE(fort_get_word_flags(fort, NULL));
	FORT_ENSURE(fort_swap(fort, NULL));
	return fort_ndrop(fort, 1);
}

static fort_err_t
fort_word_flag_set(fort_t* fort, fort_word_t* word)
{
	(void)word;

	return fort_set_word_flags(fort, NULL);
}

static fort_err_t
fort_word_code_set(fort_t* fort, fort_word_t* word)
{
	(void)word;

	fort_word_t* code_word;
	FORT_ENSURE(fort_as_word(fort, 0, &code_word));
	fort_native_fn_t code = code_word->code;
	FORT_ENSURE(fort_ndrop(fort, 1));

	return fort_set_word_code(fort, NULL, code);
}

fort_err_t
fort_load_builtins(fort_t* fort)
{
	// Arithmetic
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("+"), &fort_add, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("="), &fort_equal, 0));

	// Compilation
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("state"), &fort_state_get, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("state!"), &fort_state_set, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("immediate"), &fort_immediate, FORT_WORD_IMMEDIATE | FORT_WORD_COMPILE_ONLY));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("compile-only"), &fort_compile_only, FORT_WORD_IMMEDIATE | FORT_WORD_COMPILE_ONLY));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("compile"), &fort_compile, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF(":"), &fort_colon, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF(";"), &fort_semicolon, FORT_WORD_IMMEDIATE | FORT_WORD_COMPILE_ONLY));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("'"), &fort_tick, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("current-word"), &fort_current_word_get, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("current-word!"), &fort_current_word_set, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("quote"), &fort_quote, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("(return)"), &fort_return, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("(exec-colon)"), &fort_exec_colon, 0));

	// Scanner
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("scan-until-char"), &fort_scan_until_char, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("scan-buf"), &fort_scan_buf, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("clear-scan-buf"), &fort_clear_scan_buf, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("next-char"), &fort_get_next_char, 0));

	// Stack
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("pick"), &fort_f_pick, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("ndrop"), &fort_f_ndrop, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("roll"), &fort_f_roll, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("swap"), &fort_swap, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("."), &fort_print, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF(".s"), &fort_print_stack, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("reset"), &fort_f_reset, 0));

	// Word manipulation
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-create"), &fort_word_create, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-push"), &fort_word_push, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-delete"), &fort_word_delete, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-at"), &fort_word_get_at, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-at!"), &fort_word_set_at, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-length"), &fort_word_length, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-name"), &fort_word_name_get, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-name!"), &fort_word_name_set, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-register"), &fort_word_register, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-code!"), &fort_word_code_set, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-flag"), &fort_word_flag_get, 0));
	FORT_ENSURE(fort_create_word(fort->ctx, FORT_STRING_REF("word-flag!"), &fort_word_flag_set, 0));

	fort_string_ref_t core = {
		.ptr = (void*)fort_builtins_fs_data,
		.length = fort_builtins_fs_size
	};
	FORT_ENSURE(fort_interpret_string(fort, core, FORT_STRING_REF("<core>")));

	return FORT_OK;
}
