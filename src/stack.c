#include "internal.h"
#include <bk/array.h>
#include <fort/utils.h>

fort_err_t
fort_stack_address(fort_t* fort, fort_int_t index, fort_cell_t** cellp)
{
	fort_int_t stack_size = bk_array_len(fort->param_stack);
	index = index >= 0 ? stack_size - index - 1 : -index - 1;
	FORT_ASSERT(index >= 0, FORT_ERR_UNDERFLOW);
	FORT_ASSERT(index < stack_size, FORT_ERR_OVERFLOW);

	*cellp = fort->param_stack + index;

	return FORT_OK;
}

fort_err_t
fort_push(fort_t* fort, fort_cell_t value)
{
	bk_array_push(fort->param_stack, value);
	return FORT_OK;
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
	return fort_push(fort, (fort_cell_t){
		.type = FORT_STRING,
		.data = { .ref = str }
	});
}

fort_err_t
fort_ndrop(fort_t* fort, fort_int_t count)
{
	FORT_ASSERT(count >= 0, FORT_ERR_UNDERFLOW);

	fort_int_t stack_size = bk_array_len(fort->param_stack);
	FORT_ASSERT(count <= stack_size, FORT_ERR_UNDERFLOW);

	bk_array_resize(fort->param_stack, stack_size - count);

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

	FORT_ENSURE(fort_pick(fort, n));

	fort_int_t num_elems = n + 1;
	fort_cell_t* stack_end = fort->param_stack + bk_array_len(fort->param_stack);
	memmove(
		stack_end - num_elems - 1,
		stack_end - num_elems,
		sizeof(fort_cell_t) * num_elems
	);

	return fort_ndrop(fort, 1);
}

fort_int_t
fort_get_stack_size(fort_t* fort)
{
	return (fort_int_t)(bk_array_len(fort->param_stack));
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
	FORT_ASSERT(cell->type == FORT_STRING, FORT_ERR_TYPE);

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
			return FORT_ERR_TYPE;
	}
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
			return FORT_ERR_TYPE;
	}
}

fort_err_t
fort_xmove(fort_t* src_fort, fort_t* dst_fort, fort_int_t index)
{
	FORT_ASSERT(src_fort->ctx == dst_fort->ctx, FORT_ERR_TYPE);

	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(src_fort, index, &cell));
	return fort_push(dst_fort, *cell);
}
