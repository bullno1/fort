#include "internal.h"
#include <bk/array.h>
#include <fort-utils.h>

static fort_err_t
fort_push_stack_frame(fort_t* fort, fort_word_t* word)
{
	fort_stack_frame_t* stack_frame = bk_array_alloc(fort->return_stack);
	stack_frame->word = word;
	stack_frame->pc = word->data;
	stack_frame->max_pc = word->data + bk_array_len(word->data) - 1;

	return FORT_OK;
}

BK_INLINE fort_err_t
fort_exec_cell(fort_t* fort, fort_cell_t cell)
{
	fort_word_t* word;
	switch(cell.type)
	{
		case FORT_XT:
			word = cell.data.ref;
			return word->code(fort, word);
		case FORT_TICK:
			cell.type = FORT_XT;
			word = cell.data.ref;
			return fort_push(fort, cell);
		default:
			return fort_push(fort, cell);
	}
}

static fort_err_t
fort_exec_loop(fort_t* fort)
{
	for(;;)
	{
		size_t return_stack_depth = bk_array_len(fort->return_stack);
		FORT_ASSERT(return_stack_depth > 0, FORT_ERR_UNDERFLOW);

		fort_stack_frame_t* current_stack_frame = &fort->return_stack[return_stack_depth - 1];
		const fort_cell_t* pc = current_stack_frame->pc;
		FORT_ASSERT(pc <= current_stack_frame->max_pc, FORT_ERR_OVERFLOW);

		++current_stack_frame->pc;
		FORT_ENSURE(fort_exec_cell(fort, *pc));
	}
}

fort_err_t
fort_exec_colon(fort_t* fort, fort_word_t* word)
{
	size_t return_stack_depth = bk_array_len(fort->return_stack);
	if(return_stack_depth == 0)
	{
		FORT_ENSURE(fort_push_stack_frame(fort, fort->return_to_native));
	}

	FORT_ENSURE(fort_push_stack_frame(fort, word));

	if(return_stack_depth == 0)
	{
		fort_err_t err = fort_exec_loop(fort);
		if(err == FORT_SWITCH) { err = FORT_OK; }

		return err;
	}
	else
	{
		return FORT_OK;
	}
}
