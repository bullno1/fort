#ifndef FORT_INTERNAL_H
#define FORT_INTERNAL_H

#include <fort.h>
#include <ugc/ugc.h>
#include <bk/dlist.h>
#include "vendor/khash.h"

#define FORT_NUMERIC_BIN_OPS(X) \
	X(add, +) \
	X(sub, -) \
	X(mul, *) \
	X(div, /) \
	X(lt, <) \
	X(gt, >) \
	X(lte, <=) \
	X(gte, >=)

typedef struct fort_input_state_s fort_input_state_t;
typedef struct fort_stack_frame_s fort_stack_frame_t;
typedef struct fort_string_s fort_string_t;
typedef struct fort_strpool_entry_s fort_strpool_entry_t;
typedef struct fort_cell_s fort_cell_t;

KHASH_DECLARE(fort_strpool, fort_strpool_entry_t, char)
typedef khash_t(fort_strpool) fort_strpool_t;

KHASH_DECLARE(fort_dict, fort_cell_t, fort_cell_t)
typedef khash_t(fort_dict) fort_dict_t;

struct fort_cell_s
{
	fort_cell_type_t type;

	union
	{
		fort_int_t integer;
		fort_real_t real;
		void* ref;
	} data;
};

struct fort_input_state_s
{
	struct bk_file_s* input;
	fort_location_t location;
	char last_char;
};

struct fort_string_s
{
	fort_int_t length;
	char ptr[];
};

struct fort_word_s
{
	fort_string_t* name;

	fort_int_t flags;
	fort_native_fn_t code;
	BK_ARRAY(fort_cell_t) data;
	BK_ARRAY(uint8_t) opcodes;
};

struct fort_stack_frame_s
{
	const fort_word_t* word;
	const fort_word_t* pinned_word;
	const fort_cell_t* pc;
	const fort_cell_t* max_pc;
};

struct fort_ctx_s
{
	fort_ctx_config_t config;

	ugc_t gc;
	fort_dict_t dict;
	fort_strpool_t strpool;
	bk_dlist_link_t forts;
	fort_word_t* exit;
	fort_word_t* switch_;
};

struct fort_s
{
	bk_dlist_link_t ctx_link;

	fort_config_t config;
	fort_ctx_t* ctx;
	fort_cell_t *sp, *sp_min, *sp_max;
	fort_stack_frame_t *fp, *fp_min, *fp_max;
	BK_ARRAY(char) scan_buf;
	fort_word_t* current_word;
	fort_input_state_t input_state;
	fort_state_t state;
	fort_int_t exec_loop_level;
};

// parse

fort_err_t
fort_parse_number(fort_string_ref_t string, fort_cell_t* value);

// stack

fort_err_t
fort_push(fort_t* fort, fort_cell_t value);

fort_err_t
fort_stack_address(fort_t* fort, fort_int_t index, fort_cell_t** cellp);

fort_err_t
fort_stack_top(fort_t* fort, fort_cell_t** cellp);

fort_err_t
fort_as_word(fort_t* fort, fort_int_t index, fort_word_t** value);

// dict

fort_word_t*
fort_find_internal(fort_ctx_t* ctx, fort_string_ref_t name);

void
fort_reset_dict(fort_ctx_t* ctx);

fort_err_t
fort_push_word_data_internal(
	fort_ctx_t* ctx, fort_word_t* word, fort_cell_t value
);

fort_err_t
fort_begin_word(
	fort_ctx_t* ctx,
	fort_string_ref_t name,
	fort_native_fn_t code,
	fort_word_t** wordp
);

fort_err_t
fort_end_word(fort_ctx_t* ctx, fort_word_t* word);

// exec

fort_err_t
fort_exec_colon(fort_t* fort, fort_word_t* word);

fort_err_t
fort_exit(fort_t* fort, fort_word_t* word);

fort_err_t
fort_switch(fort_t* fort, fort_word_t* word);

fort_err_t
fort_compile_word(fort_ctx_t* ctx, fort_word_t* word);

// strpool

fort_err_t
fort_strpool_alloc(
	fort_ctx_t* ctx,
	fort_string_ref_t ref,
	fort_string_t** strp
);

fort_err_t
fort_strpool_release(fort_ctx_t* ctx, fort_string_t* str);

fort_err_t
fort_strpool_check(
	fort_ctx_t* ctx,
	fort_string_ref_t ref,
	fort_string_t** strp
);

// gc

fort_err_t
fort_gc_alloc(
	fort_ctx_t* ctx,
	size_t size,
	size_t alignment,
	fort_cell_type_t type,
	void** memp
);

void
fort_gc_visit_ptr(fort_ctx_t* ctx, void* mem);

void
fort_gc_visit_cell(fort_ctx_t* ctx, fort_cell_t cell);

void
fort_gc_add_ptr_ref(fort_ctx_t* ctx, void* src, void* dest);

void
fort_gc_add_cell_ref(fort_ctx_t* ctx, void* src, fort_cell_t dest);

void
fort_gc_scan(ugc_t* gc, ugc_header_t* ugc_header);

void
fort_gc_release(ugc_t* gc, ugc_header_t* ugc_header);

#endif
