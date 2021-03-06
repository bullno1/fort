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
	ugc_init(&ctx->gc, &fort_gc_scan, fort_gc_release);
	ctx->gc.userdata = ctx;

	FORT_ENSURE(fort_begin_word(ctx, FORT_STRING_REF("exit"), &fort_exit, &ctx->exit));
	FORT_ENSURE(fort_end_word(ctx, ctx->exit));

	fort_word_t* switch_ = NULL;
	FORT_ENSURE(fort_begin_word(ctx, FORT_STRING_REF("(switch)"), &fort_switch, &switch_));

	FORT_ENSURE(fort_begin_word(ctx, FORT_STRING_REF("switch"), fort_exec_colon, &ctx->switch_));
	FORT_ENSURE(
		fort_push_word_data_internal(
			ctx,
			ctx->switch_,
			(fort_cell_t){
				.type = FORT_XT,
				.data = { .ref = switch_ }
			}
		)
	);

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
	fort_end_word(ctx, ctx->exit);
}

static fort_err_t
fort_alloc_stack(
	bk_allocator_t* allocator,
	size_t cell_size,
	size_t depth,
	void** minp,
	void** maxp
)
{
	char* stack = bk_unsafe_malloc(allocator, cell_size * depth);
	FORT_ASSERT(stack != NULL, FORT_ERR_OOM);

	*minp = stack;
	*maxp = stack + cell_size * (depth - 1);

	return FORT_OK;
}

fort_err_t
fort_create(fort_ctx_t* ctx, const fort_config_t* config, fort_t** fortp)
{
	fort_t* fort = BK_UNSAFE_NEW(ctx->config.allocator, fort_t);
	FORT_ASSERT(fort != NULL, FORT_ERR_OOM);

	*fort = (fort_t){
		.config = *config,
		.ctx = ctx,
		.scan_buf = bk_array_create(ctx->config.allocator, char, 16),
		.state = FORT_STATE_INTERPRETING
	};

	// TODO: check stack size < INT_MAX

	FORT_ENSURE(
		fort_alloc_stack(
			ctx->config.allocator,
			sizeof(fort_cell_t),
			config->param_stack_size,
			(void**)&fort->sp_min, (void**)&fort->sp_max
		)
	);

	FORT_ENSURE(
		fort_alloc_stack(
			ctx->config.allocator,
			sizeof(fort_cell_t),
			config->local_stack_size,
			(void**)&fort->lp_min, (void**)&fort->lp_max
		)
	);

	FORT_ENSURE(
		fort_alloc_stack(
			ctx->config.allocator,
			sizeof(fort_stack_frame_t),
			config->return_stack_size,
			(void**)&fort->fp_min, (void**)&fort->fp_max
		)
	);

	FORT_ENSURE(fort_reset(fort));
	bk_dlist_append(&ctx->forts, &fort->ctx_link);

	*fortp = fort;
	return FORT_OK;
}

void
fort_destroy(fort_t* fort)
{
	bk_dlist_unlink(&fort->ctx_link);
	bk_array_destroy(fort->scan_buf);
	bk_free(fort->ctx->config.allocator, fort->sp_min);
	bk_free(fort->ctx->config.allocator, fort->lp_min);
	bk_free(fort->ctx->config.allocator, fort->fp_min);
	bk_free(fort->ctx->config.allocator, fort);
}

fort_err_t
fort_reset(fort_t* fort)
{
	FORT_ASSERT(fort->exec_loop_level == 0, FORT_ERR_INVALID);

	fort->sp = fort->sp_min;
	fort->fp = fort->fp_min;
	bk_array_clear(fort->scan_buf);
	fort->current_word = NULL;
	*fort->fp = (fort_stack_frame_t){
		.local_base = fort->lp_min,
		.lp = fort->lp_min
	};
	fort->state = FORT_STATE_INTERPRETING;

	return FORT_OK;
}

fort_ctx_t*
fort_get_ctx(fort_t* fort) { return fort->ctx; }

fort_state_t
fort_get_state(fort_t* fort) { return fort->state; }

void
fort_set_state(fort_t* fort, fort_state_t state) { fort->state = state; }
