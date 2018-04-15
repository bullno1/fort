#include "internal.h"
#include <bk/allocator.h>
#include <bk/array.h>

static void
fort_unload_all(fort_t* fort)
{
	for(fort_word_t* itr = fort->dict; itr != NULL;)
	{
		fort_word_t* next = itr->previous;
		bk_free(fort->config.allocator, itr);
		itr = next;
	}

	fort->dict = NULL;
}

fort_err_t
fort_create(const fort_config_t* config, fort_t** fortp)
{
	fort_t* fort = BK_NEW(config->allocator, fort_t);

	*fort = (fort_t){
		.config = *config,
		.param_stack = bk_array_create(config->allocator, fort_cell_t, 16),
		.return_stack = bk_array_create(config->allocator, fort_stack_frame_t, 16),
		.scan_buf = bk_array_create(config->allocator, char, 16)
	};

	fort_reset(fort);

	*fortp = fort;
	return FORT_OK;
}

void
fort_destroy(fort_t* fort)
{
	fort_unload_all(fort);
	strpool_term(&fort->strpool);
	bk_array_destroy(fort->scan_buf);
	bk_array_destroy(fort->return_stack);
	bk_array_destroy(fort->param_stack);
	bk_free(fort->config.allocator, fort);
}

void
fort_reset(fort_t* fort)
{
	if(fort->strpool.memctx != NULL) { strpool_term(&fort->strpool); }
	strpool_config_t strpool_cfg = strpool_default_config;
	strpool_cfg.memctx = fort->config.allocator;
	strpool_cfg.ignore_case = 1;
	strpool_init(&fort->strpool, &strpool_cfg);

	bk_array_clear(fort->param_stack);
	bk_array_clear(fort->return_stack);

	fort_unload_all(fort);
	fort_load_builtins(fort);
}
