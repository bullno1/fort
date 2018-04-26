#include "internal.h"
#include <bk/array.h>
#include <fort/utils.h>

#define FORT_FIRST(X, Y) F(X)

#define FORT_BASIC_OPS(X) \
	X(f_pick) \
	X(f_ndrop) \
	X(f_roll) \
	X(swap)

#define FORT_BRANCH_OPS(X) \
	X(jmp) \
	X(jmp0) \
	X(exec_colon) \
	X(exit)

#define FORT_OP_NAME(OP) FORT_OP_##OP,
#define FORT_NUMERIC_BIN_OP_NAME(NAME, OP) FORT_OP_##NAME,

#define FORT_DECLARE_BASIC_OP_FN(OP) \
	fort_err_t fort_##OP (fort_t*, fort_word_t*);

#define FORT_DECLARE_NUMERIC_BIN_OP_FN(NAME, OP) \
	fort_err_t fort_##NAME (fort_t*, fort_word_t*);

FORT_BASIC_OPS(FORT_DECLARE_BASIC_OP_FN)
FORT_BRANCH_OPS(FORT_DECLARE_BASIC_OP_FN)
FORT_NUMERIC_BIN_OPS(FORT_DECLARE_NUMERIC_BIN_OP_FN)

enum fort_ops_e
{
	// Weird naming convention to stay consistent
	FORT_OP_push,
	FORT_OP_unquote,
	FORT_OP_call,
	FORT_BASIC_OPS(FORT_OP_NAME)
	FORT_BRANCH_OPS(FORT_OP_NAME)
	FORT_NUMERIC_BIN_OPS(FORT_NUMERIC_BIN_OP_NAME)
};

static fort_err_t
fort_push_stack_frame(fort_t* fort, fort_word_t* word)
{
	fort_stack_frame_t* new_frame = fort-> fp + 1;
	FORT_ASSERT(new_frame <= fort->fp_max, FORT_ERR_OVERFLOW);
	new_frame->word = word;
	new_frame->pinned_word = word;
	new_frame->pc = word->data;
	new_frame->max_pc = word->data + bk_array_len(word->data) - 1;
	fort->fp = new_frame;

	return FORT_OK;
}

static fort_err_t
fort_exec_loop(fort_t* fort)
{
	const fort_cell_t *pc, *operand;
	fort_stack_frame_t* fp;
	const uint8_t* opcode;

#define FORT_LOAD_FRAME() \
	fp = fort->fp; \
	pc = fp->pc; \
	opcode = &fp->word->opcodes[pc - fp->word->data]; \
	FORT_ASSERT(fp->word->opcodes != NULL, FORT_ERR_INVALID); // TODO: FORT_ERR_INTERNAL

#define FORT_SAVE_FRAME() \
	fp->pc = pc;

#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wpedantic"
#endif

#define FORT_MAKE_BASIC_OP_LABEL(OP) &&lbl_##OP,
#define FORT_MAKE_NUMERIC_BIN_OP_LABEL(NAME, OP) &&lbl_##NAME,
	void* dispatch_table[] = {
		&&lbl_push,
		&&lbl_unquote,
		&&lbl_call,
		FORT_BASIC_OPS(FORT_MAKE_BASIC_OP_LABEL)
		FORT_BRANCH_OPS(FORT_MAKE_BASIC_OP_LABEL)
		FORT_NUMERIC_BIN_OPS(FORT_MAKE_NUMERIC_BIN_OP_LABEL)
		&&lbl_invalid
	};

#define FORT_CASE_BASIC_OP(OP) case FORT_OP_##OP: goto lbl_##OP;
#define FORT_CASE_NUMERIC_BIN_OP(NAME, OP) \
		case FORT_OP_##NAME: goto lbl_##NAME;

	// TODO: FORT_ERR_INTERNAL
#define FORT_NEXT() \
	operand = pc++; \
	goto *dispatch_table[*(opcode++)];

#define FORT_EXEC_BASIC_OP(OP) \
		lbl_##OP: FORT_ENSURE(fort_##OP(fort, operand->data.ref)); FORT_NEXT()
#define FORT_EXEC_NUMERIC_BIN_OP(NAME, OP) \
		lbl_##NAME: FORT_ENSURE(fort_##NAME(fort, operand->data.ref)); FORT_NEXT()

	FORT_LOAD_FRAME()
	FORT_NEXT()

	lbl_push:
		FORT_ENSURE(fort_push(fort, *operand));
		FORT_NEXT()
	lbl_unquote:
		{
			fort_cell_t copy = *operand;
			copy.type = FORT_XT;
			FORT_ENSURE(fort_push(fort, copy));
		}
		FORT_NEXT()
	lbl_call:
		{
			fort_word_t* called_word = operand->data.ref;
			FORT_SAVE_FRAME();
			FORT_ENSURE(called_word->code(fort, called_word));
		}
		FORT_LOAD_FRAME();
		FORT_NEXT()
	lbl_jmp:
		{
			fort_int_t delta = pc->data.integer;
			pc += delta;
			opcode += delta;
		}
		FORT_NEXT()
	lbl_jmp0:
		{
			fort_int_t jmp_delta = pc->data.integer;

			fort_int_t cond;
			FORT_ENSURE(fort_pop_bool(fort, &cond));

			fort_int_t delta = cond ? 1 : jmp_delta;
			pc += delta;
			opcode += delta;
		}
		FORT_NEXT()
	lbl_exec_colon:
		FORT_SAVE_FRAME()
		fort_push_stack_frame(fort, operand->data.ref);
		FORT_LOAD_FRAME()
		FORT_NEXT()
	lbl_exit:
		--fort->fp;
		FORT_LOAD_FRAME()
		FORT_NEXT()
	FORT_BASIC_OPS(FORT_EXEC_BASIC_OP)
	FORT_NUMERIC_BIN_OPS(FORT_EXEC_NUMERIC_BIN_OP)
	lbl_invalid:
		return FORT_ERR_INVALID;
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
fort_get_cell_opcode(fort_cell_t cell, enum fort_ops_e* opcode)
{
	switch(cell.type)
	{
		case FORT_NULL:
		case FORT_REAL:
		case FORT_INTEGER:
		case FORT_STRING:
			*opcode = FORT_OP_push;
			return FORT_OK;
		case FORT_TICK:
			*opcode = FORT_OP_unquote;
			return FORT_OK;
		case FORT_XT:
			{
				fort_word_t* word = cell.data.ref;
				fort_native_fn_t code = word->code;

#define FORT_CHECK_BASIC_OPS(OP) \
				if(code == fort_##OP) { *opcode = FORT_OP_##OP; } else
#define FORT_CHECK_NUMERIC_BIN_OPS(NAME, OP) \
				if(code == fort_##NAME) { *opcode = FORT_OP_##NAME; } else

				FORT_BASIC_OPS(FORT_CHECK_BASIC_OPS)
				FORT_BRANCH_OPS(FORT_CHECK_BASIC_OPS)
				FORT_NUMERIC_BIN_OPS(FORT_CHECK_NUMERIC_BIN_OPS)
				{
					*opcode = FORT_OP_call;
				}

				return FORT_OK;
			}
		default:
			return FORT_ERR_INVALID;
	}
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
	fort->fp->pinned_word = word;
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

	// TODO: maybe call fort_compile_word here?
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

fort_err_t
fort_compile_word(fort_ctx_t* ctx, fort_word_t* word)
{
	(void)ctx;

	FORT_ASSERT(word->opcodes == NULL && word->data != NULL, FORT_ERR_INVALID);

	word->opcodes = bk_array_create(
		ctx->config.allocator, uint8_t, bk_array_len(word->data)
	);

	bk_array_foreach(fort_cell_t, itr, word->data)
	{
		enum fort_ops_e opcode;
		FORT_ENSURE(fort_get_cell_opcode(*itr, &opcode));
		bk_array_push(word->opcodes, opcode);
	}

	// TODO: check for invalid jumps

	return FORT_OK;
}
