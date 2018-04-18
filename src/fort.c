#include "internal.h"
#include <bk/allocator.h>
#include <bk/assert.h>
#include <bk/array.h>
#include <fort/utils.h>

fort_err_t
fort_create_ctx(const fort_ctx_config_t* config, fort_ctx_t** ctxp)
{
	fort_ctx_t* ctx = BK_UNSAFE_NEW(config->allocator, fort_ctx_t);
	FORT_ASSERT(ctx != NULL, FORT_ERR_OOM);

	*ctx = (fort_ctx_t){ .config = *config };
	kh_init(fort_strpool, config->allocator, &ctx->strpool);
	kh_init(fort_dict, config->allocator, &ctx->dict);
	bk_dlist_init(&ctx->forts);
	ugc_init(&ctx->gc, &fort_gc_scan_internal, fort_gc_release_internal);
	ctx->gc.userdata = ctx;

	fort_reset_ctx(ctx);

	*ctxp = ctx;

	return FORT_OK;
}

void
fort_destroy_ctx(fort_ctx_t* ctx)
{
	BK_ASSERT(bk_dlist_is_empty(&ctx->forts), "Destroying context with live forts");
	ugc_release_all(&ctx->gc);
	kh_cleanup(fort_dict, &ctx->dict);
	kh_cleanup(fort_strpool, &ctx->strpool);
	bk_free(ctx->config.allocator, ctx);
}

void
fort_reset_ctx(fort_ctx_t* ctx)
{
	kh_clear(fort_dict, &ctx->dict);
}

fort_err_t
fort_create(fort_ctx_t* ctx, const fort_config_t* config, fort_t** fortp)
{
	fort_t* fort = BK_UNSAFE_NEW(ctx->config.allocator, fort_t);
	FORT_ASSERT(fort != NULL, FORT_ERR_OOM);

	*fort = (fort_t){
		.config = *config,
		.ctx = ctx,
		.param_stack = bk_array_create(ctx->config.allocator, fort_cell_t, 16),
		.return_stack = bk_array_create(ctx->config.allocator, fort_stack_frame_t, 16),
		.scan_buf = bk_array_create(ctx->config.allocator, char, 16)
	};

	bk_dlist_append(&ctx->forts, &fort->ctx_link);
	fort_reset(fort);

	*fortp = fort;
	return FORT_OK;
}

void
fort_destroy(fort_t* fort)
{
	bk_dlist_unlink(&fort->ctx_link);
	bk_array_destroy(fort->scan_buf);
	bk_array_destroy(fort->return_stack);
	bk_array_destroy(fort->param_stack);
	bk_free(fort->ctx->config.allocator, fort);
}

void
fort_reset(fort_t* fort)
{
	bk_array_clear(fort->return_stack);
	bk_array_clear(fort->param_stack);
}

fort_ctx_t*
fort_ctx(fort_t* fort) { return fort->ctx; }

fort_int_t
fort_state(fort_t* fort) { return fort->compiling; }
