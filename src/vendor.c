#ifndef FORT_USE_EXTERNAL_BK
#define BK_IMPLEMENTATION
#endif

#define _GNU_SOURCE // For strcasecmp
#include <bk/array.h>
#include <bk/allocator.h>
#include <bk/fs/mem.h>
#include <bk/assert.h>
#include <bk/printf.h>

#define STRPOOL_IMPLEMENTATION
#define STRPOOL_MALLOC(ctx, size) (bk_malloc(ctx, size))
#define STRPOOL_FREE(ctx, ptr) (bk_free(ctx, ptr))
#define STRPOOL_ASSERT(condition) BK_ASSERT(condition, "strpool error")
#include "vendor/strpool.h"
