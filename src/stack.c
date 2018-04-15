#include "internal.h"
#include <bk/array.h>
#include <fort-utils.h>

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
fort_pop(fort_t* fort, fort_cell_t* value)
{
	fort_int_t stack_size = bk_array_len(fort->param_stack);
	FORT_ASSERT(stack_size > 0, FORT_ERR_UNDERFLOW);

	--stack_size;

	*value = fort->param_stack[stack_size];
	bk_array_resize(fort->param_stack, stack_size);

	return FORT_OK;
}

fort_err_t
fort_peek(fort_t* fort, fort_int_t index, fort_cell_t* value)
{
	fort_int_t stack_size = bk_array_len(fort->param_stack);
	index = index >= 0 ? stack_size - index - 1 : -index - 1;

	FORT_ASSERT(index >= 0, FORT_ERR_UNDERFLOW);

	*value = fort->param_stack[index];

	return FORT_OK;
}

fort_int_t
fort_stack_size(fort_t* fort)
{
	return (fort_int_t)(bk_array_len(fort->param_stack));
}
