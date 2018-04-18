#ifndef FORT_INTERNAL_H
#define FORT_INTERNAL_H

#include <fort.h>
#include <ugc/ugc.h>
#include "vendor/khash.h"
#define XXH_NAMESPACE fort_
#include <xxHash/xxhash.h>

typedef struct fort_state_s fort_state_t;
typedef struct fort_stack_frame_s fort_stack_frame_t;
typedef struct fort_string_s fort_string_t;
typedef struct fort_strpool_entry_s fort_strpool_entry_t;

KHASH_DECLARE(fort_strpool, fort_strpool_entry_t, char)
typedef khash_t(fort_strpool) fort_strpool_t;

KHASH_DECLARE(fort_dict, fort_cell_t, fort_cell_t)
typedef khash_t(fort_dict) fort_dict_t;

struct fort_state_s
{
	struct bk_file_s* input;
	fort_location_t location;
	char last_char;

	unsigned interpreting: 1;
};

struct fort_string_s
{
	fort_int_t length;
	char ptr[];
};

struct fort_word_s
{
	const fort_string_t* name;

	fort_native_fn_t code;
	BK_ARRAY(fort_cell_t) data;

	unsigned immediate: 1;
	unsigned compile_only: 1;
};

struct fort_stack_frame_s
{
	const fort_word_t* word;
	const fort_cell_t* pc;
	const fort_cell_t* max_pc;
};

struct fort_ctx_s
{
	fort_ctx_config_t config;

	ugc_t gc;
	fort_dict_t dict;
	fort_strpool_t strpool;
};

struct fort_s
{
	fort_config_t config;
	fort_ctx_t* ctx;
	BK_ARRAY(fort_cell_t) param_stack;
	BK_ARRAY(fort_stack_frame_t) return_stack;
	BK_ARRAY(char) scan_buf;
	fort_word_t* current_word;
	fort_word_t* last_word;
	fort_word_t* return_to_native;
	fort_state_t state;
};

fort_err_t
fort_parse_number(fort_string_ref_t string, fort_cell_t* value);

fort_err_t
fort_push(fort_t* fort, fort_cell_t value);

fort_word_t*
fort_find_internal(fort_ctx_t* ctx, fort_string_ref_t name);

fort_err_t
fort_begin_word(fort_t* fort, fort_string_ref_t name, fort_native_fn_t code);

fort_err_t
fort_end_word(fort_t* fort);

void
fort_reset_dict(fort_ctx_t* ctx);

fort_err_t
fort_compile_internal(fort_t* fort, fort_cell_t cell);

fort_err_t
fort_exec_colon(fort_t* fort, fort_word_t* word);

fort_err_t
fort_strpool_alloc(
	fort_ctx_t* ctx,
	fort_string_ref_t ref,
	const fort_string_t** strp
);

fort_err_t
fort_strpool_release(fort_ctx_t* ctx, const fort_string_t* str);

fort_err_t
fort_strpool_check(
	fort_ctx_t* ctx,
	fort_string_ref_t ref,
	const fort_string_t** strp
);

fort_err_t
fort_gc_alloc(
	fort_ctx_t* ctx,
	size_t size,
	size_t alignment,
	fort_cell_type_t type,
	void** memp
);

void
fort_gc_scan(fort_ctx_t* ctx, void* mem);

void
fort_gc_scan_cell(fort_ctx_t* ctx, fort_cell_t cell);

void
fort_gc_add_ref(fort_ctx_t* ctx, void* src, void* dest);

void
fort_gc_add_cell_ref(fort_ctx_t* ctx, void* src, fort_cell_t dest);

void
fort_gc_scan_internal(ugc_t* gc, ugc_header_t* ugc_header);

void
fort_gc_release_internal(ugc_t* gc, ugc_header_t* ugc_header);

#endif
