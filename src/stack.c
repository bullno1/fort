#include "internal.h"
#include <bk/array.h>

fort_err_t
fort_push_integer(fort_t* fort, fort_int_t value)
{
	bk_array_push(fort->param_stack_types, FORT_INTEGER);
	bk_array_push(fort->param_stack_values, (fort_cell_t){ .integer = value });
	return FORT_OK;
}

fort_err_t
fort_push_real(fort_t* fort, fort_real_t value)
{
	bk_array_push(fort->param_stack_types, FORT_REAL);
	bk_array_push(fort->param_stack_values, (fort_cell_t){ .real = value });
	return FORT_OK;
}

fort_err_t
fort_pop(fort_t* fort, fort_cell_type_t* type, fort_cell_t* value)
{
	fort_int_t stack_size = bk_array_len(fort->param_stack_types);
	if(stack_size == 0) { return FORT_ERR_UNDERFLOW; }

	--stack_size;

	*type = fort->param_stack_types[stack_size];
	*value = fort->param_stack_values[stack_size];
	bk_array_resize(fort->param_stack_types, stack_size);
	bk_array_resize(fort->param_stack_values, stack_size);

	return FORT_OK;
}

fort_err_t
fort_peek(
	fort_t* fort, fort_int_t index, fort_cell_type_t* type, fort_cell_t* value
)
{
	fort_int_t stack_size = bk_array_len(fort->param_stack_types);
	index = index >= 0 ? stack_size - index - 1 : -index - 1;

	if(index < 0) { return FORT_ERR_UNDERFLOW; }

	*type = fort->param_stack_types[index];
	*value = fort->param_stack_values[index];

	return FORT_OK;
}

fort_int_t
fort_stack_size(fort_t* fort)
{
	return (fort_int_t)(bk_array_len(fort->param_stack_types));
}
