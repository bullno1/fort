#include "internal.h"
#include <bk/array.h>
#include <fort/utils.h>

BK_INLINE fort_err_t
fort_exec_cell(fort_t* fort, fort_cell_t cell)
{
	switch(cell.type)
	{
		case FORT_XT:
			{
				fort_word_t* word = cell.data.ref;
				return word->code(fort, word);
			}
		case FORT_TICK:
			cell.type = FORT_XT;
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
		const fort_cell_t* pc = fort->current_frame.pc++;
		FORT_ASSERT(pc <= fort->current_frame.max_pc, FORT_ERR_OVERFLOW);
		FORT_ENSURE(fort_exec_cell(fort, *pc));
	}
}

static fort_err_t
fort_enter_exec_loop(fort_t* fort)
{
	++fort->exec_loop_level;
	fort_err_t err = fort_exec_loop(fort);
	if(err == FORT_SWITCH) { err = FORT_OK; }
	--fort->exec_loop_level;

	return err;
}

static fort_err_t
fort_push_stack_frame(fort_t* fort, fort_word_t* word)
{
	bk_array_push(fort->return_stack, fort->current_frame);
	fort->current_frame.word = word;
	fort->current_frame.pc = word->data;
	fort->current_frame.max_pc = word->data + bk_array_len(word->data) - 1;

	return FORT_OK;
}

fort_err_t
fort_execute(fort_t* fort)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, 0, &cell));
	FORT_ASSERT(cell->type == FORT_XT, FORT_ERR_TYPE);

	fort_word_t* word = cell->data.ref;
	FORT_ENSURE(fort_push_stack_frame(fort, fort->ctx->switch_));
	// Pin the word to the stack frame before popping to prevent GC
	fort->current_frame.word = word;
	FORT_ENSURE(fort_ndrop(fort, 1));

	FORT_ENSURE(word->code(fort, word));

	return fort_enter_exec_loop(fort);
}

fort_err_t
fort_exec_colon(fort_t* fort, fort_word_t* word)
{
	fort_int_t exec_loop_level = fort->exec_loop_level;

	if(exec_loop_level == 0)
	{
		FORT_ENSURE(fort_push_stack_frame(fort, fort->ctx->switch_));
	}

	fort_push_stack_frame(fort, word);

	if(exec_loop_level == 0)
	{
		return fort_enter_exec_loop(fort);
	}
	else
	{
		return FORT_OK;
	}
}

fort_err_t
fort_return(fort_t* fort, fort_word_t* word)
{
	(void)word;

	size_t return_stack_depth = bk_array_len(fort->return_stack);
	FORT_ASSERT(return_stack_depth > 0, FORT_ERR_UNDERFLOW);

	fort->current_frame = fort->return_stack[return_stack_depth - 1];
	bk_array_resize(fort->return_stack, return_stack_depth - 1);

	FORT_ASSERT(word->data != NULL && bk_array_len(word->data) >= 1, FORT_ERR_OVERFLOW);
	return word->data[0].data.integer;
}
