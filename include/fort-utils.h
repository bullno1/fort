#ifndef FORT_UTILS_H
#define FORT_UTILS_H

#include <string.h>
#include <fort.h>

#define FORT_STRING_REF(STR) \
	((fort_string_ref_t){ .ptr = STR, .length = sizeof(STR) - 1 })

inline fort_string_ref_t
fort_string_ref(const char* string)
{
	return (fort_string_ref_t){
		.ptr = string,
		.length = strlen(string)
	};
}

#endif
