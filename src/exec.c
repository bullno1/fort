#include "internal.h"
#include <bk/array.h>
#include <fort/utils.h>

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
		const fort_cell_t* pc = fort->current_frame.pc++;
		FORT_ASSERT(pc <= fort->current_frame.max_pc, FORT_ERR_OVERFLOW);
		FORT_ENSURE(fort_exec_cell(fort, *pc));
	}
}

static void
fort_push_stack_frame(fort_t* fort, fort_word_t* word)
{
	bk_array_push(fort->return_stack, fort->current_frame);
	fort->current_frame.word = word;
	fort->current_frame.pc = word->data;
	fort->current_frame.max_pc = word->data + bk_array_len(word->data) - 1;
}

fort_err_t
fort_execute(fort_t* fort)
{
	fort_cell_t* cell;
	FORT_ENSURE(fort_stack_address(fort, 0, &cell));
	// TODO: pin cell in case of gc
	FORT_ENSURE(fort_ndrop(fort, 1));

	return fort_exec_cell(fort, *cell);
}

fort_err_t
fort_exec_colon(fort_t* fort, fort_word_t* word)
{
	int top_is_native = fort->current_frame.word == NULL;

	if(top_is_native)
	{
		if(fort->return_to_native == NULL)
		{
			fort->return_to_native = fort_find_internal(
				fort->ctx, FORT_STRING_REF("return-to-native")
			);
			FORT_ASSERT(fort->return_to_native != NULL, FORT_ERR_NOT_FOUND);
		}
		fort_push_stack_frame(fort, fort->return_to_native);
	}

	fort_push_stack_frame(fort, word);

	if(top_is_native)
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
