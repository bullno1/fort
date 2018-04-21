#ifndef FORT_UTILS_H
#define FORT_UTILS_H

#include <string.h>
#include <fort.h>

#define FORT_STRING_REF(STR) \
	((fort_string_ref_t){ .ptr = STR, .length = sizeof(STR) - 1 })
#define FORT_ASSERT(COND, ERR) \
	do { if(!(COND)) { return (ERR); } } while(0)
#define FORT_ENSURE(OP) \
	do { fort_err_t err = (OP); if(err != FORT_OK) { return err; } } while(0)

BK_INLINE fort_string_ref_t
fort_string_ref(const char* string)
{
	return (fort_string_ref_t){
		.ptr = string,
		.length = strlen(string)
	};
}

BK_INLINE fort_err_t
fort_pop_integer(fort_t* fort, fort_int_t* value)
{
	FORT_ENSURE(fort_as_integer(fort, 0, value));
	return fort_ndrop(fort, 1);
}

BK_INLINE fort_err_t
fort_pop_real(fort_t* fort, fort_real_t* value)
{
	FORT_ENSURE(fort_as_real(fort, 0, value));
	return fort_ndrop(fort, 1);
}

#endif
