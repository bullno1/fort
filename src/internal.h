#ifndef FORT_INTERNAL_H
#define FORT_INTERNAL_H

#include <fort.h>
#include "vendor/strpool.h"

typedef struct fort_state_s fort_state_t;
typedef struct fort_stack_frame_s fort_stack_frame_t;

struct fort_state_s
{
	struct bk_file_s* input;
	fort_location_t location;
	char last_char;

	unsigned interpreting: 1;
};

struct fort_word_s
{
	struct fort_word_s* previous;
	STRPOOL_U64 name;

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

struct fort_s
{
	fort_config_t config;
	BK_ARRAY(fort_cell_t) param_stack;
	BK_ARRAY(fort_stack_frame_t) return_stack;
	BK_ARRAY(char) scan_buf;
	fort_word_t* dict;
	fort_word_t* current_word;
	fort_word_t* return_to_native;
	strpool_t strpool;

	fort_state_t state;
};

fort_word_t*
fort_find_internal(fort_t* fort, fort_string_ref_t name);

fort_err_t
fort_begin_compile(fort_t* fort);

fort_err_t
fort_end_compile(fort_t* fort);

fort_err_t
fort_parse_number(fort_string_ref_t string, fort_cell_t* value);

fort_err_t
fort_push(fort_t* fort, fort_cell_t value);

fort_err_t
fort_begin_word(fort_t* fort, fort_string_ref_t name, fort_native_fn_t code);

fort_err_t
fort_end_word(fort_t* fort);

void
fort_reset_dict(fort_t* fort);

fort_err_t
fort_compile_internal(fort_t* fort, fort_cell_t cell);

fort_err_t
fort_exec_colon(fort_t* fort, fort_word_t* word);

#endif
