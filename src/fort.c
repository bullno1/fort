#include "internal.h"
#include <bk/allocator.h>
#include <bk/array.h>

fort_err_t
fort_create(fort_config_t* config, fort_t** fortp)
{
	fort_t* fort = BK_NEW(config->allocator, fort_t);

	*fort = (fort_t){
		.config = *config,
		.param_stack_types = bk_array_create(config->allocator, uint8_t, 16),
		.param_stack_values = bk_array_create(config->allocator, fort_cell_t, 16),
		.scan_buf = bk_array_create(config->allocator, char, 16)
	};
	fort_reset(fort);

	*fortp = fort;
	return FORT_OK;
}

void
fort_destroy(fort_t* fort)
{
	bk_array_destroy(fort->scan_buf);
	bk_array_destroy(fort->param_stack_types);
	bk_array_destroy(fort->param_stack_values);
	bk_free(fort->config.allocator, fort);
}

void
fort_reset(fort_t* fort)
{
	bk_array_clear(fort->param_stack_types);
	bk_array_clear(fort->param_stack_values);
}
