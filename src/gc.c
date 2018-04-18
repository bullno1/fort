#include "internal.h"
#include <bk/array.h>
#include <bk/allocator.h>
#include <fort/utils.h>

typedef struct fort_gc_header_s fort_gc_header_t;

struct fort_gc_header_s
{
	ugc_header_t ugc;
	uint8_t type;
	uint8_t offset;
};

BK_INLINE fort_gc_header_t*
fort_gc_header(void* mem)
{
	char* body = mem;
	char* header = body - body[-1];
	return (fort_gc_header_t*)header;
}

BK_INLINE void*
fort_gc_get_cell_ref(fort_cell_t cell)
{
	if(0
		|| cell.type == FORT_STRING
		|| cell.type == FORT_XT
		|| cell.type == FORT_TICK
	)
	{
		return cell.data.ref;
	}
	else
	{
		return NULL;
	}
}

fort_err_t
fort_gc_alloc(
	fort_ctx_t* ctx,
	size_t size,
	size_t alignment,
	fort_cell_type_t type,
	void** memp
)
{
	size_t alloc_size = 0
		+ sizeof(fort_gc_header_t)
		+ alignment - 1
		+ size;

	char* mem = bk_unsafe_malloc(ctx->config.allocator, alloc_size);
	FORT_ASSERT(mem != NULL, FORT_ERR_OOM);

	fort_gc_header_t* header = (void*)mem;
	header->type = type;

	char* header_end = mem + sizeof(fort_gc_header_t);
	char* body = BK_ALIGN_PTR(header_end, alignment);
	header->offset = body[-1] = body - (char*)header;

	*memp = body;
	ugc_register(&ctx->gc, &header->ugc);

	return FORT_OK;
}

void
fort_gc_scan(fort_ctx_t* ctx, void* mem)
{
	if(mem == NULL) { return; }

	ugc_visit(&ctx->gc, &fort_gc_header(mem)->ugc);
}

void
fort_gc_scan_cell(fort_ctx_t* ctx, fort_cell_t cell)
{
	fort_gc_scan(ctx, fort_gc_get_cell_ref(cell));
}

void
fort_gc_add_ref(fort_ctx_t* ctx, void* src, void* dest)
{
	if(src == NULL || dest == NULL) { return; }

	ugc_add_ref(&ctx->gc, &fort_gc_header(src)->ugc, &fort_gc_header(dest)->ugc);
}

void
fort_gc_add_cell_ref(fort_ctx_t* ctx, void* src, fort_cell_t dest)
{
	fort_gc_add_ref(ctx, src, fort_gc_get_cell_ref(dest));
}

static void
fort_gc_scan_word(ugc_t* gc, fort_word_t* word)
{
	fort_gc_scan(gc->userdata, (void*)word->name);
	if(word->data != NULL)
	{
		bk_array_foreach(fort_cell_t, itr, word->data)
		{
			fort_gc_scan_cell(gc->userdata, *itr);
		}
	}
}

static void
fort_gc_scan_dict(fort_ctx_t* ctx, fort_dict_t* dict)
{
	kh_foreach(itr, dict)
	{
		fort_gc_scan_cell(ctx, kh_key(dict, itr));
		fort_gc_scan_cell(ctx, kh_value(dict, itr));
	}
}

void
fort_gc_scan_internal(ugc_t* gc, ugc_header_t* ugc_header)
{
	if(ugc_header == NULL)
	{
		fort_ctx_t* ctx = gc->userdata;
		fort_gc_scan_dict(ctx, &ctx->dict);
	}
	else
	{
		fort_gc_header_t* header = BK_CONTAINER_OF(ugc_header, fort_gc_header_t, ugc);
		void* body = (char*)header + header->offset;

		if(header->type == FORT_TICK || header->type == FORT_XT)
		{
			fort_gc_scan_word(gc, body);
		}
	}
}

void
fort_gc_release_internal(ugc_t* gc, ugc_header_t* ugc_header)
{
	fort_gc_header_t* header = BK_CONTAINER_OF(ugc_header, fort_gc_header_t, ugc);
	fort_ctx_t* ctx = gc->userdata;
	void* body = (char*)header + header->offset;

	if(header->type == FORT_TICK || header->type == FORT_XT)
	{
		fort_word_t* word = body;
		if(word->data != NULL) { bk_array_destroy(word->data); }
	}
	else if(header->type == FORT_STRING)
	{
		fort_strpool_release(ctx, body);
	}

	bk_free(ctx->config.allocator, header);
}
