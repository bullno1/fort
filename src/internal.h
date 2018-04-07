#ifndef FORT_INTERNAL_H
#define FORT_INTERNAL_H

#include <fort.h>
#include "vendor/strpool.h"

typedef struct fort_state_s fort_state_t;

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
};

struct fort_s
{
	fort_config_t config;
	BK_ARRAY(uint8_t) param_stack_types;
	BK_ARRAY(fort_cell_t) param_stack_values;
	BK_ARRAY(char) scan_buf;
	fort_word_t* dict;
	strpool_t strpool;

	fort_state_t state;
};

const fort_word_t*
fort_find_internal(fort_t* fort, fort_string_ref_t name);

void
fort_load_builtins(fort_t* fort);

#endif
