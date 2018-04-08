#include "internal.h"
#include <fort-utils.h>
#include <bk/allocator.h>

static void
fort_add_native_fn(fort_t* fort, fort_string_ref_t name, fort_native_fn_t fn)
{
	// TODO: assert/check string length against INT_MAX
	fort_word_t* word = BK_NEW(fort->config.allocator, fort_word_t);
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

static fort_err_t
fort_numeric_2pop(
	fort_t* fort, fort_cell_type_t* type, fort_cell_t* lhs, fort_cell_t* rhs
)
{
	fort_cell_type_t lhs_t, rhs_t;
	fort_err_t err;

	if((err = fort_pop(fort, &rhs_t, rhs)) != FORT_OK) { return err; }
	if((err = fort_pop(fort, &lhs_t, lhs)) != FORT_OK) { return err; }

	if(!(fort_is_numeric(lhs_t) && fort_is_numeric(rhs_t))) { return FORT_ERR_TYPE; }

	if(lhs_t == FORT_INTEGER && rhs_t == FORT_INTEGER)
	{
		*type = FORT_INTEGER;
	}
	else
	{
		*type = FORT_REAL;

		if(lhs_t == FORT_INTEGER) { lhs->real = (fort_real_t)lhs->integer; }
		if(rhs_t == FORT_INTEGER) { rhs->real = (fort_real_t)rhs->integer; }
	}

	return FORT_OK;
}

static fort_err_t
fort_add(fort_t* fort, const fort_word_t* word)
{
	(void)word;

	fort_cell_type_t type;
	fort_cell_t lhs, rhs;
	fort_err_t err;

	if((err = fort_numeric_2pop(fort, &type, &lhs, &rhs)) != FORT_OK)
	{
		return err;
	}

	if(type == FORT_REAL)
	{
		return fort_push_real(fort, lhs.real + rhs.real);
	}
	else
	{
		return fort_push_integer(fort, lhs.integer + rhs.integer);
	}
}

void
fort_load_builtins(fort_t* fort)
{
	fort_add_native_fn(fort, FORT_STRING_REF("+"), &fort_add);
}
