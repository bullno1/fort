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
		fort_stack_frame_t* current_frame = fort->fp;
		const fort_cell_t* pc = current_frame->pc++;
		FORT_ASSERT(pc <= current_frame->max_pc, FORT_ERR_OVERFLOW);
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
	fort_stack_frame_t* old_frame = fort->fp;
	fort_stack_frame_t* new_frame = old_frame + 1;
	FORT_ASSERT(new_frame <= fort->fp_max, FORT_ERR_OVERFLOW);

	new_frame->word = word;
	new_frame->pc = word->data;
	fort_int_t word_length = word->data ? bk_array_len(word->data) : 0;
	FORT_ASSERT(word_length > 0, FORT_ERR_INVALID);
	new_frame->max_pc = word->data + word_length - 1;

	new_frame->local_base = new_frame->lp = old_frame->lp;

	fort->fp = new_frame;

	return FORT_OK;
}

fort_err_t
fort_execute(fort_t* fort)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_top(fort, &cell));
	FORT_ASSERT(cell->type == FORT_XT, FORT_ERR_INVALID);

	fort_word_t* word = cell->data.ref;
	FORT_ENSURE(fort_push_stack_frame(fort, fort->ctx->switch_));
	// Pin the word to the stack frame before popping to prevent GC
	fort->fp->word = word;
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

	FORT_ENSURE(fort_push_stack_frame(fort, word));

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
fort_exit(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ASSERT(fort->fp > fort->fp_min, FORT_ERR_UNDERFLOW);

	--fort->fp;

	return FORT_OK;
}

fort_err_t
fort_switch(fort_t* fort, fort_word_t* word)
{
	(void)word;

	FORT_ASSERT(fort->fp > fort->fp_min, FORT_ERR_UNDERFLOW);

	--fort->fp;

	return FORT_SWITCH;
}
