#include "internal.h"
#include <bk/array.h>
#include <bk/allocator.h>
#include <fort/utils.h>
#define XXH_PRIVATE_API
#define XXH_NAMESPACE fort_
#include <xxHash/xxhash.h>

BK_INLINE khint_t
fort_cell_hash(fort_cell_t cell)
{
	// Hash type and data separately because there can be padding junk in between
	XXH32_state_t state;
	XXH32_reset(&state, __LINE__);
	XXH32_update(&state, &cell.type, sizeof(cell.type));
	XXH32_update(&state, &cell.data, sizeof(cell.data));
	return XXH32_digest(&state);
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

static fort_err_t
fort_push_word(fort_t* fort, fort_word_t* word)
{
	return fort_push(fort, (fort_cell_t){
		.type = FORT_XT,
		.data = { .ref = word }
	});
}

static fort_err_t
fort_normalize_word_data_index(fort_word_t* word, fort_int_t* indexp)
{
	FORT_ASSERT(word->data != NULL, FORT_ERR_OVERFLOW);

	fort_int_t data_size = bk_array_len(word->data);
	fort_int_t index = *indexp;
	index = index >= 0 ? index : data_size + index;
	FORT_ASSERT(index >= 0, FORT_ERR_UNDERFLOW);
	FORT_ASSERT(index < data_size, FORT_ERR_OVERFLOW);

	*indexp = index;

	return FORT_OK;
}

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
	fort_gc_add_ptr_ref(ctx, current_word, current_word->name);
	current_word->code = code;
	current_word->flags = 0;

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
	fort_int_t flags
)
{
	fort_word_t* word = NULL;
	FORT_ENSURE(fort_begin_word(ctx, name, code, &word));
	word->flags = flags;
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

	// TODO: ensure word size can't grow over INT_MAX
	bk_array_push(word->data, cell);
	fort_gc_add_cell_ref(ctx, word, cell);

	return FORT_OK;
}

fort_err_t
fort_create_unnamed_word(fort_t* fort)
{
	fort_word_t* word = NULL;
	FORT_ENSURE(fort_begin_word(fort->ctx, FORT_STRING_REF(""), &fort_push_word, &word));
	return fort_push_word(fort, word);
}

FORT_DECL fort_err_t
fort_get_word_data(fort_t* fort, fort_word_t* word, fort_int_t index)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	FORT_ENSURE(fort_normalize_word_data_index(word, &index));

	return fort_push(fort, word->data[index]);
}

FORT_DECL fort_err_t
fort_set_word_data(fort_t* fort, fort_word_t* word, fort_int_t index)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_top(fort, &cell));

	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 1, &word)); }

	FORT_ENSURE(fort_normalize_word_data_index(word, &index));

	word->data[index] = *cell;
	fort_gc_add_cell_ref(fort->ctx, word, *cell);

	return fort_ndrop(fort, 1);
}

FORT_DECL fort_err_t
fort_delete_word_data(fort_t* fort, fort_word_t* word, fort_int_t index)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	FORT_ENSURE(fort_normalize_word_data_index(word, &index));
	bk_array_remove(word->data, (size_t)index);

	return FORT_OK;
}

FORT_DECL fort_err_t
fort_push_word_data(fort_t* fort, fort_word_t* word)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_top(fort, &cell));

	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 1, &word)); }

	FORT_ENSURE(fort_push_word_data_internal(fort->ctx, word, *cell));

	return fort_ndrop(fort, 1);
}

FORT_DECL fort_err_t
fort_clear_word_data(fort_t* fort, fort_word_t* word)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }
	if(word->data == NULL) { return FORT_OK; }

	bk_array_clear(word->data);

	return FORT_OK;
}

fort_err_t
fort_get_word_length(fort_t* fort, fort_word_t* word)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	if(word->data == NULL)
	{
		return fort_push_integer(fort, 0);
	}
	else
	{
		return fort_push_integer(fort, bk_array_len(word->data));
	}
}

FORT_DECL fort_err_t
fort_get_word_name(fort_t* fort, fort_word_t* word)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	return fort_push(fort, (fort_cell_t){
		.type = FORT_STRING,
		.data = { .ref = word->name }
	});
}

FORT_DECL fort_err_t
fort_set_word_name(fort_t* fort, fort_word_t* word)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_top(fort, &cell));
	FORT_ASSERT(cell->type == FORT_STRING, FORT_ERR_TYPE);

	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 1, &word)); }

	word->name = cell->data.ref;
	fort_gc_add_ptr_ref(fort->ctx, word, cell->data.ref);

	return fort_ndrop(fort, 1);
}

FORT_DECL fort_err_t
fort_get_word_code(fort_t* fort, fort_word_t* word, fort_native_fn_t* code)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	*code = word->code;

	return FORT_OK;
}

FORT_DECL fort_err_t
fort_set_word_code(fort_t* fort, fort_word_t* word, fort_native_fn_t code)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	word->code = code;

	return FORT_OK;
}

FORT_DECL fort_err_t
fort_get_word_flags(fort_t* fort, fort_word_t* word)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	return fort_push_integer(fort, word->flags);
}

FORT_DECL fort_err_t
fort_set_word_flags(fort_t* fort, fort_word_t* word)
{
	fort_int_t flags;
	FORT_ENSURE(fort_as_integer(fort, 0, &flags));
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 1, &word)); }

	return fort_ndrop(fort, 1);
}

FORT_DECL fort_err_t
fort_register_word(fort_t* fort, fort_word_t* word)
{
	if(word == NULL) { FORT_ENSURE(fort_as_word(fort, 0, &word)); }

	FORT_ASSERT(word->name != NULL, FORT_ERR_TYPE); // TODO: FORT_ERR_INVALID

	return FORT_OK;
}
