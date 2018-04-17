#ifndef FORT_USE_EXTERNAL_BK
#define BK_IMPLEMENTATION
#include <bk/array.h>
#include <bk/allocator.h>
#include <bk/fs/mem.h>
#include <bk/assert.h>
#endif

#define UGC_IMPLEMENTATION
#include <ugc/ugc.h>

#define XXH_NAMESPACE fort_
#include <xxHash/xxhash.c>
