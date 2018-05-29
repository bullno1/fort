#include "internal.h"
#include <fort/utils.h>

static fort_err_t
fort_local_address(fort_t* fort, fort_int_t id, fort_cell_t** localp)
{
	FORT_ASSERT(id >= 0, FORT_ERR_UNDERFLOW);
	fort_cell_t* local = fort->fp->local_base + id;
	FORT_ASSERT(local < fort->fp->lp, FORT_ERR_OVERFLOW);
	*localp = local;

	return FORT_OK;
}

fort_err_t
fort_declare_local(fort_t* fort, fort_int_t* id)
{
	fort_stack_frame_t* fp = fort->fp;
	FORT_ASSERT(fp->lp <= fort->lp_max, FORT_ERR_OVERFLOW);

	fp->lp->type = FORT_NULL;
	*id = fp->lp - fp->local_base;
	++fp->lp;

	return FORT_OK;
}

fort_err_t
fort_set_local(fort_t* fort, fort_int_t id, fort_int_t index)
{
	fort_cell_t *value, *local;
	FORT_ENSURE(fort_stack_address(fort, index, &value));
	FORT_ENSURE(fort_local_address(fort, id, &local));

	*local = *value;

	return FORT_OK;
}

fort_err_t
fort_get_local(fort_t* fort, fort_int_t id)
{
	fort_cell_t* local;
	FORT_ENSURE(fort_local_address(fort, id, &local));

	return fort_push(fort, *local);
}
