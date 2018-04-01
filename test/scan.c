#include <munit/munit.h>
#include "test_helper.h"
#include <fort.h>
#include <bk/default_allocator.h>
#include <bk/stdstream.h>
#include <bk/fs/mem.h>
#include "../src/internal.h"

#define MAKE_TOKEN(LEXEME, START_LINE, START_COL, END_LINE, END_COL) \
	{ .lexeme = LEXEME \
	, .length = sizeof(LEXEME) - 1 \
	, .location = { .start = { .line = START_LINE, .column = START_COL }  \
		          , .end = { .line = START_COL, .column = END_COL } \
	              } \
	}

static void*
setup(const MunitParameter params[], void* userdata)
{
	(void)params;
	(void)userdata;

	fort_config_t fort_cfg = {
		.allocator = bk_default_allocator,
		.output = bk_stdout
	};
	fort_t* fort;

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(&fort_cfg, &fort));

	return fort;
}

static void
teardown(void* fixture)
{
	fort_destroy(fixture);
}

static MunitResult
next_token(const MunitParameter params[], void* fixture)
{
	(void)params;
	fort_t* fort = fixture;

	char input[] =
		"(\n"
		"  )\n"
		"50.6 \"hi hi \" \r\n"
		"; wat";

	fort_token_t expected_tokens[] = {
		MAKE_TOKEN("(", 1, 1, 1, 1),
		MAKE_TOKEN(")", 2, 3, 2, 3),
		MAKE_TOKEN("50.6", 3, 1, 3, 4),
		MAKE_TOKEN("\"hi", 3, 6, 3, 8),
		MAKE_TOKEN("hi", 3, 10, 3, 11),
		MAKE_TOKEN("\"", 3, 13, 3, 13),
		MAKE_TOKEN(";", 4, 1, 4, 1),
		MAKE_TOKEN("wat", 4, 6, 4, 8),
	};

	bk_mem_file_t mem_file;
	bk_file_t* file = bk_mem_fs_wrap_fixed(&mem_file, input, strlen(input));
	fort->interpreter_state.input = file;

	size_t num_tokens = BK_STATIC_ARRAY_LEN(expected_tokens);
	for(size_t i = 0; i < num_tokens; ++i)
	{
		fort_token_t token;
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_next_token(fort, &token));

		munit_assert_size(expected_tokens[i].length, ==, token.length);
	}

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/next_token",
		.test = next_token,
		.setup = setup,
		.tear_down = teardown
	},
	{ 0 }
};

MunitSuite scan = {
	.prefix = "/scan",
	.tests = tests
};
