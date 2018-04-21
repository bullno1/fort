#include "internal.h"
#include <bk/array.h>
#include <bk/allocator.h>
#include <fort/utils.h>

BK_INLINE khint_t
fort_cell_hash(fort_cell_t cell)
{
	return XXH32(&cell, sizeof(cell), __LINE__);
}

BK_INLINE khint_t
fort_cell_equal(fort_cell_t lhs, fort_cell_t rhs)
{
	return lhs.type == rhs.type && lhs.data.ref == rhs.data.ref;
}

__KHASH_IMPL(
	fort_dict,
	,
	fort_cell_t,
	fort_cell_t,
	1,
	fort_cell_hash,
	fort_cell_equal
)

fort_err_t
fort_begin_word(
	fort_ctx_t* ctx,
	fort_string_ref_t name,
	fort_native_fn_t code,
	fort_word_t** wordp
)
{
	fort_word_t* current_word = *wordp;
	if(current_word != NULL)
	{
		if(current_word->data) { bk_array_clear(current_word->data); }
	}
	else
	{
		FORT_ENSURE(fort_gc_alloc(
			ctx,
			sizeof(fort_word_t),
			BK_ALIGN_OF(fort_word_t),
			FORT_XT,
			(void**)&current_word
		));

		current_word->data = NULL;
	}

	FORT_ENSURE(fort_strpool_alloc(ctx, name, &current_word->name));
	current_word->code = code;
	current_word->immediate = 0;
	current_word->compile_only = 0;

	*wordp = current_word;

	return FORT_OK;
}

fort_err_t
fort_end_word(fort_ctx_t* ctx, fort_word_t* word)
{
	fort_cell_t key = {
		.type = FORT_STRING,
		.data = { .ref = word->name }
	};
	fort_cell_t value = {
		.type = FORT_XT,
		.data = { .ref = word }
	};

	int ret;
	khint_t itr = kh_put(fort_dict, &ctx->dict, key, &ret);
	FORT_ASSERT(ret != -1, FORT_ERR_OOM);
	kh_value(&ctx->dict, itr) = value;

	return FORT_OK;
}

fort_err_t
fort_create_word(
	fort_ctx_t* ctx,
	fort_string_ref_t name,
	fort_native_fn_t code,
	unsigned immediate,
	unsigned compile_only
)
{
	fort_word_t* word = NULL;
	FORT_ENSURE(fort_begin_word(ctx, name, code, &word));
	word->immediate = immediate;
	word->compile_only = compile_only;
	return fort_end_word(ctx, word);
}

fort_word_t*
fort_find_internal(fort_ctx_t* ctx, fort_string_ref_t name)
{
	fort_string_t* str;
	if(fort_strpool_check(ctx, name, &str) != FORT_OK) { return NULL; }

	fort_cell_t key = {
		.type = FORT_STRING,
		.data = { .ref = str }
	};

	khiter_t itr = kh_get(fort_dict, &ctx->dict, key);
	if(itr == kh_end(&ctx->dict)) { return NULL; }

	return kh_value(&ctx->dict, itr).data.ref;
}

fort_err_t
fort_compile_internal(fort_t* fort, fort_cell_t cell)
{
	fort_word_t* current_word = fort->current_word;
	FORT_ASSERT(current_word != NULL, FORT_ERR_NOT_FOUND);

	if(current_word->data == NULL)
	{
		current_word->data = bk_array_create(fort->ctx->config.allocator, fort_cell_t, 1);
	}

	bk_array_push(current_word->data, cell);
	fort_gc_add_cell_ref(fort->ctx, current_word, cell);

	return FORT_OK;
}

fort_err_t
fort_find(fort_t* fort, fort_string_ref_t name)
{
	fort_word_t* word = fort_find_internal(fort->ctx, name);
	FORT_ASSERT(word != NULL, FORT_ERR_NOT_FOUND);
	return fort_push(fort, (fort_cell_t){
		.type = FORT_XT,
		.data = { .ref = word }
	});
}

fort_err_t
fort_push_word_data_internal(
	fort_ctx_t* ctx, fort_word_t* word, fort_cell_t cell
)
{
	if(word->data == NULL)
	{
		word->data = bk_array_create(ctx->config.allocator, fort_cell_t, 1);
	}

	bk_array_push(word->data, cell);
	fort_gc_add_cell_ref(ctx, word, cell);

	return FORT_OK;
}
