#include "internal.h"
#include <limits.h>
#include <bk/allocator.h>
#include <fort-utils.h>

struct fort_string_align_helper_s
{
	fort_int_t length;
	char ptr[1];
};

struct fort_strpool_entry_s
{
	khint_t hash; // pre-calculated
	unsigned owned: 1;
	union
	{
		fort_string_ref_t ref;
		fort_string_t* str;
	} content;
};

BK_INLINE void
fort_strpool_select(fort_strpool_entry_t entry, const char** strp, fort_int_t* lenp)
{
	if(entry.owned)
	{
		*strp = entry.content.str->ptr;
		*lenp = entry.content.str->length;
	}
	else
	{
		*strp = entry.content.ref.ptr;
		*lenp = entry.content.ref.length;
	}
}

BK_INLINE khint_t
fort_strpool_entry_hash(fort_strpool_entry_t entry)
{
	return entry.hash;
}

BK_INLINE khint_t
fort_strpool_entry_equal(fort_strpool_entry_t lhs, fort_strpool_entry_t rhs)
{
	const char *str_l, *str_r;
	fort_int_t len_l, len_r;

	fort_strpool_select(lhs, &str_l, &len_l);
	fort_strpool_select(rhs, &str_r, &len_r);

	return len_l == len_r && memcmp(str_l, str_r, (size_t)len_l) == 0;
}

__KHASH_IMPL(
	fort_strpool,
	,
	fort_strpool_entry_t,
	char,
	0,
	fort_strpool_entry_hash,
	fort_strpool_entry_equal
)

fort_err_t
fort_strpool_alloc(
	fort_ctx_t* ctx,
	fort_string_ref_t ref,
	const fort_string_t** strp
)
{
	FORT_ASSERT(ref.length <= INT_MAX, FORT_ERR_OOM);

	fort_strpool_entry_t entry = {
		.hash = XXH32(ref.ptr, ref.length, __LINE__),
		.owned = 0,
		.content = { .ref = ref }
	};

	khint_t itr = kh_get(fort_strpool, &ctx->strpool, entry);
	if(itr == kh_end(&ctx->strpool))
	{
		size_t alloc_size = sizeof(fort_string_t) + ref.length + 1;
		fort_string_t* str;
		FORT_ENSURE(fort_gc_alloc(
			ctx,
			alloc_size,
			BK_ALIGN_OF(struct fort_string_align_helper_s),
			FORT_STRING,
			(void*)&str
		));

		str->length = ref.length;
		memcpy(str->ptr, ref.ptr, ref.length);
		str->ptr[ref.length] = '\0';

		entry.owned = 1;
		entry.content.str = str;

		int ret;
		kh_put(fort_strpool, &ctx->strpool, entry, &ret);
		FORT_ASSERT(ret != -1, FORT_ERR_OOM);

		*strp = str;
		return FORT_OK;
	}
	else
	{
		*strp = kh_key(&ctx->strpool, itr).content.str;
		return FORT_OK;
	}
}
