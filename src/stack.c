#include "internal.h"
#include <bk/array.h>
#include <fort/utils.h>

fort_int_t
fort_get_stack_size(fort_t* fort)
{
	return (fort_int_t)(fort->sp - fort->sp_min);
}

fort_err_t
fort_stack_address(fort_t* fort, fort_int_t index, fort_cell_t** cellp)
{
	fort_cell_t* origin = index >= 0 ? fort->sp : fort->sp_min;
	fort_cell_t* addr = origin - index - 1;
	FORT_ASSERT(fort->sp_min <= addr , FORT_ERR_UNDERFLOW);
	FORT_ASSERT(addr < fort->sp, FORT_ERR_OVERFLOW);

	*cellp = addr;

	return FORT_OK;
}

fort_err_t
fort_stack_top(fort_t* fort, fort_cell_t** cellp)
{
	FORT_ASSERT(fort->sp > fort->sp_min, FORT_ERR_OVERFLOW);
	*cellp = fort->sp - 1;

	return FORT_OK;
}

fort_err_t
fort_push(fort_t* fort, fort_cell_t value)
{
	FORT_ASSERT(fort->sp <= fort->sp_max, FORT_ERR_OVERFLOW);
	*(fort->sp++) = value;
	return FORT_OK;
}

fort_err_t
fort_push_null(fort_t* fort)
{
	return fort_push(fort, (fort_cell_t){
		.type = FORT_NULL,
		.data = { .ref = NULL }
	});
}

fort_err_t
fort_push_integer(fort_t* fort, fort_int_t value)
{
	return fort_push(fort, (fort_cell_t){
		.type = FORT_INTEGER,
		.data = { .integer = value }
	});
}

fort_err_t
fort_push_real(fort_t* fort, fort_real_t value)
{
	return fort_push(fort, (fort_cell_t){
		.type = FORT_REAL,
		.data = { .real = value }
	});
}

fort_err_t
fort_push_string(fort_t* fort, fort_string_ref_t value)
{
	fort_string_t* str;
	FORT_ENSURE(fort_strpool_alloc(fort->ctx, value, &str));
	if(str == NULL)
	{
		return fort_push_null(fort);
	}
	else
	{
		return fort_push(fort, (fort_cell_t){
			.type = FORT_STRING,
			.data = { .ref = str }
		});
	}
}

fort_err_t
fort_ndrop(fort_t* fort, fort_int_t count)
{
	FORT_ASSERT(count >= 0, FORT_ERR_INVALID);

	fort_cell_t* new_sp = fort->sp - count;
	FORT_ASSERT(fort->sp_min <= new_sp, FORT_ERR_UNDERFLOW);

	fort->sp = new_sp;

	return FORT_OK;
}

fort_err_t
fort_pick(fort_t* fort, fort_int_t index)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, index, &cell));
	return fort_push(fort, *cell);
}

fort_err_t
fort_move(fort_t* fort, fort_int_t src, fort_int_t dst)
{
	fort_cell_t *src_cell, *dst_cell;
	FORT_ENSURE(fort_stack_address(fort, src, &src_cell));
	FORT_ENSURE(fort_stack_address(fort, dst, &dst_cell));

	*dst_cell = *src_cell;

	return FORT_OK;
}

fort_err_t
fort_roll(fort_t* fort, fort_int_t n)
{
	FORT_ASSERT(n >= 0, FORT_ERR_UNDERFLOW);

	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, n, &cell));
	fort_cell_t rolled_value = *cell;

	memmove(cell, cell + 1, sizeof(fort_cell_t) * n);
	fort->sp[-1] = rolled_value;

	return FORT_OK;
}

fort_err_t
fort_get_type(fort_t* fort, fort_int_t index, fort_cell_type_t* typep)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, index, &cell));
	*typep = cell->type;

	return FORT_OK;
}

fort_err_t
fort_as_string(fort_t* fort, fort_int_t index, fort_string_ref_t* value)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, index, &cell));
	FORT_ASSERT(cell->type == FORT_STRING, FORT_ERR_INVALID);

	fort_string_t* string = cell->data.ref;
	*value = (fort_string_ref_t){
		.length = string->length,
		.ptr = string->ptr,
	};

	return FORT_OK;
}

fort_err_t
fort_as_integer(fort_t* fort, fort_int_t index, fort_int_t* value)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, index, &cell));

	switch(cell->type)
	{
		case FORT_REAL:
			*value = (fort_int_t)cell->data.real;
			return FORT_OK;
		case FORT_INTEGER:
			*value = (fort_int_t)cell->data.integer;
			return FORT_OK;
		default:
			return FORT_ERR_INVALID;
	}
}

fort_err_t
fort_as_bool(fort_t* fort, fort_int_t index, fort_int_t* value)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, index, &cell));

	*value = cell->data.ref != NULL;
	return FORT_OK;
}

fort_err_t
fort_as_real(fort_t* fort, fort_int_t index, fort_real_t* value)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, index, &cell));

	switch(cell->type)
	{
		case FORT_REAL:
			*value = (fort_real_t)cell->data.real;
			return FORT_OK;
		case FORT_INTEGER:
			*value = (fort_real_t)cell->data.integer;
			return FORT_OK;
		default:
			return FORT_ERR_INVALID;
	}
}

fort_err_t
fort_as_word(fort_t* fort, fort_int_t index, fort_word_t** value)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, index, &cell));
	FORT_ASSERT(cell->type == FORT_XT, FORT_ERR_INVALID);

	*value = cell->data.ref;

	return FORT_OK;
}

fort_err_t
fort_xmove(fort_t* src_fort, fort_t* dst_fort, fort_int_t index)
{
	FORT_ASSERT(src_fort->ctx == dst_fort->ctx, FORT_ERR_INVALID);

	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(src_fort, index, &cell));
	return fort_push(dst_fort, *cell);
}
