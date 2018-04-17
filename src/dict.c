#include "internal.h"
#include <bk/array.h>
#include <bk/allocator.h>
#include <fort-utils.h>

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
fort_begin_word(fort_t* fort, fort_string_ref_t name, fort_native_fn_t code)
{
	if(name.length > INT_MAX) { return FORT_ERR_OOM; }

	fort_word_t* current_word = fort->current_word;
	if(current_word != NULL)
	{
		if(current_word->data) { bk_array_clear(current_word->data); }
	}
	else
	{
		FORT_ENSURE(fort_gc_alloc(
			fort->ctx,
			sizeof(fort_word_t),
			BK_ALIGN_OF(fort_word_t),
			FORT_XT,
			(void*)&current_word
		));

		current_word->data = NULL;
		fort->current_word = current_word;
	}

	FORT_ENSURE(fort_strpool_alloc(fort->ctx, name, &current_word->name));
	current_word->code = code;
	current_word->immediate = 0;
	current_word->compile_only = 0;

	return FORT_OK;
}

fort_err_t
fort_end_word(fort_t* fort)
{
	fort_cell_t key = {
		.type = FORT_STRING,
		.data = { .ref = (fort_string_t*)fort->current_word->name }
	};
	fort_cell_t value = {
		.type = FORT_XT,
		.data = { .ref = fort->current_word }
	};

	int ret;
	khint_t itr = kh_put(fort_dict, &fort->ctx->dict, key, &ret);
	FORT_ASSERT(ret != -1, FORT_ERR_OOM);
	kh_value(&fort->ctx->dict, itr) = value;

	fort->last_word = fort->current_word;
	fort->current_word = NULL;

	return FORT_OK;
}

fort_word_t*
fort_find_internal(fort_ctx_t* ctx, fort_string_ref_t name)
{
	kh_foreach(itr, &ctx->dict)
	{
		fort_cell_t key = kh_key(&ctx->dict, itr);
		const fort_string_t* word_name = key.data.ref;
		if(
			name.length == (size_t)word_name->length
			&& memcmp(name.ptr, word_name->ptr, name.length) == 0
		)
		{
			return kh_value(&ctx->dict, itr).data.ref;
		}
	}

	return NULL;
}
