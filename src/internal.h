#ifndef FORT_INTERNAL_H
#define FORT_INTERNAL_H

#include <fort.h>

typedef struct fort_interpreter_state_s fort_interpreter_state_t;

struct fort_interpreter_state_s
{
	struct bk_file_s* input;
};

struct fort_s
{
	fort_config_t config;
	BK_ARRAY(uint8_t) param_stack_types;
	BK_ARRAY(fort_cell_t) param_stack_values;
	BK_ARRAY(char) scan_buf;

	fort_interpreter_state_t interpreter_state;

	unsigned interpreting: 1;
};

#endif
