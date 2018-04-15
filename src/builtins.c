#include "internal.h"
#include <limits.h>
#include <fort-utils.h>
#include <bk/allocator.h>
#include <bk/assert.h>

static void
fort_add_native_fn(fort_t* fort, fort_string_ref_t name, fort_native_fn_t fn)
{
	fort_word_t* word = BK_NEW(fort->config.allocator, fort_word_t);
	BK_ASSERT(name.length <= INT_MAX, "String too long");
	word->name = strpool_inject(&fort->strpool, name.ptr, (int)name.length);
	word->code = fn;

	word->previous = fort->dict;
	fort->dict = word;
}

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
	fort_err_t err;

	if((err = fort_pop(fort, rhs)) != FORT_OK) { return err; }
	if((err = fort_pop(fort, lhs)) != FORT_OK) { return err; }

	if(!fort_is_numeric(lhs->type) || !fort_is_numeric(rhs->type))
	{
		return FORT_ERR_TYPE;
	}

	if(lhs->type != FORT_INTEGER || rhs->type != FORT_INTEGER)
	{
		if(lhs->type == FORT_INTEGER) { fort_promote_int_to_real(lhs); }
		if(rhs->type == FORT_INTEGER) { fort_promote_int_to_real(rhs); }
	}

	return FORT_OK;
}

static fort_err_t
fort_add(fort_t* fort, const fort_word_t* word)
{
	(void)word;

	fort_cell_t lhs, rhs;
	fort_err_t err;

	if((err = fort_numeric_2pop(fort, &lhs, &rhs)) != FORT_OK)
	{
		return err;
	}

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
fort_colon(fort_t* fort, const fort_word_t* word)
{
	(void)word;
	return fort_begin_compile(fort);
}

static fort_err_t
fort_semicolon(fort_t* fort, const fort_word_t* word)
{
	(void)word;
	return fort_end_compile(fort);
}

void
fort_load_builtins(fort_t* fort)
{
	fort_add_native_fn(fort, FORT_STRING_REF("+"), &fort_add);
	fort_add_native_fn(fort, FORT_STRING_REF(":"), &fort_colon);
	fort_add_native_fn(fort, FORT_STRING_REF(";"), &fort_semicolon);
}
