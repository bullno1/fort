#define _GNU_SOURCE
#include "internal.h"
#include <string.h>
#include "vendor/strpool.h"

#ifdef _MSVC
#	define FORT_STRNICMP(s1, s2, len) (strnicmp(s1, s2, len))
#else
#	define FORT_STRNICMP(s1, s2, len) (strncasecmp(s1, s2, len))
#endif

const fort_word_t*
fort_find_internal(fort_t* fort, fort_string_ref_t name)
{
	for(fort_word_t* itr = fort->dict; itr != NULL; itr = itr->previous)
	{
		int cstr_len = strpool_length(&fort->strpool, itr->name);
		if((fort_int_t)cstr_len != name.length) { continue; }

		const char* cstr = strpool_cstr(&fort->strpool, itr->name);
		if(FORT_STRNICMP(name.ptr, cstr, cstr_len) == 0)
		{
			return itr;
		}
	}

	return NULL;
}
