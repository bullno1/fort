#define _GNU_SOURCE
#include "internal.h"
#include <string.h>
#include <limits.h>
#include <bk/allocator.h>
#include <bk/array.h>
#include "vendor/strpool.h"
#include <fort-utils.h>

#ifdef _MSVC
#	define FORT_STRNICMP(s1, s2, len) (strnicmp(s1, s2, len))
#else
#	define FORT_STRNICMP(s1, s2, len) (strncasecmp(s1, s2, len))
#endif

STRPOOL_U64
fort_alloc_string(strpool_t* strpool, fort_string_ref_t str)
{
	STRPOOL_U64 handle = strpool_inject(strpool, str.ptr, (int)str.length);
	strpool_incref(strpool, handle);
	return handle;
}

void
fort_free_string(strpool_t* strpool, STRPOOL_U64 handle)
{
	if(strpool_decref(strpool, handle) == 0)
	{
		strpool_discard(strpool, handle);
	}
}

static void
fort_destroy_word(fort_t* fort, fort_word_t* word)
{
	if(word->data != NULL) { bk_array_destroy(word->data); }
	fort_free_string(&fort->strpool, word->name);
	bk_free(fort->config.allocator, word);
}

fort_err_t
fort_compile_internal(fort_t* fort, fort_cell_t cell)
{
	fort_word_t* current_word = fort->current_word;
	FORT_ASSERT(current_word != NULL, FORT_ERR_NOT_FOUND);

	if(current_word->data == NULL)
	{
		current_word->data = bk_array_create(fort->config.allocator, fort_cell_t, 1);
	}

	bk_array_push(current_word->data, cell);

	return FORT_OK;
}

void
fort_reset_dict(fort_t* fort)
{
	for(fort_word_t* itr = fort->dict; itr != NULL;)
	{
		fort_word_t* next = itr->previous;
		fort_destroy_word(fort, itr);
		itr = next;
	}

	fort->dict = NULL;
}

fort_err_t
fort_begin_word(fort_t* fort, fort_string_ref_t name, fort_native_fn_t code)
{
	if(name.length > INT_MAX) { return FORT_ERR_OOM; }

	fort_word_t* current_word = fort->current_word;
	if(current_word != NULL)
	{
		fort_free_string(&fort->strpool, current_word->name);
		bk_array_clear(current_word->data);
	}
	else
	{
		fort->current_word = current_word = BK_UNSAFE_NEW(fort->config.allocator, fort_word_t);
		if(current_word == NULL) { return FORT_ERR_OOM; }

		current_word->data = NULL;
	}

	current_word->name = fort_alloc_string(&fort->strpool, name);
	current_word->code = code;
	current_word->immediate = 0;
	current_word->compile_only = 0;

	return FORT_OK;
}

fort_err_t
fort_end_word(fort_t* fort)
{
	fort->current_word->previous = fort->dict;
	fort->dict = fort->current_word;
	fort->current_word = NULL;

	return FORT_OK;
}

fort_word_t*
fort_find_internal(fort_t* fort, fort_string_ref_t name)
{
	for(fort_word_t* itr = fort->dict; itr != NULL; itr = itr->previous)
	{
		int cstr_len = strpool_length(&fort->strpool, itr->name);
		if((fort_int_t)cstr_len != name.length) { continue; }

		const char* cstr = strpool_cstr(&fort->strpool, itr->name);
		if(FORT_STRNICMP(name.ptr, cstr, cstr_len) == 0)
		{
			return itr;
		}
	}

	return NULL;
}
