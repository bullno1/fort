#include "test_common.h"
#include "../src/internal.h"

#define MAKE_TOKEN(STR, START_LINE, START_COL, END_LINE, END_COL) \
	{ .lexeme = { .ptr = STR, .length = sizeof(STR) - 1 } \
	, .location = { .start = { .line = START_LINE, .column = START_COL }  \
		          , .end = { .line = END_LINE, .column = END_COL } \
	              } \
	}

static MunitResult
next_token(const MunitParameter params[], void* fixture)
{
	(void)params;
	fort_t* fort = fixture;

	char input[] =
		"(\n"
		"  )\r"
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
		MAKE_TOKEN("wat", 4, 3, 4, 5),
	};

	bk_mem_file_t mem_file;
	bk_file_t* file = bk_mem_fs_wrap_fixed(&mem_file, input, strlen(input));
	fort->input_state.input = file;
	fort->input_state.location = (fort_location_t){
		.line = 1, .column = 0
	};

	size_t num_tokens = BK_STATIC_ARRAY_LEN(expected_tokens);
	fort_token_t token;

	for(size_t i = 0; i < num_tokens; ++i)
	{
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_next_token(fort, &token));

		munit_logf(MUNIT_LOG_INFO, "token #%zd", i);

		munit_assert_size(expected_tokens[i].lexeme.length, ==, token.lexeme.length);
		munit_assert_memory_equal(token.lexeme.length, expected_tokens[i].lexeme.ptr, token.lexeme.ptr);
		munit_assert_uint(expected_tokens[i].location.start.line, ==, token.location.start.line);
		munit_assert_uint(expected_tokens[i].location.start.column, ==, token.location.start.column);
		munit_assert_uint(expected_tokens[i].location.end.line, ==, token.location.end.line);
		munit_assert_uint(expected_tokens[i].location.end.column, ==, token.location.end.column);
	}
	munit_assert_enum(fort_err_t, FORT_ERR_NOT_FOUND, ==, fort_next_token(fort, &token));

	return MUNIT_OK;
}

static MunitResult
next_char(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", "next-char *");

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/next_token",
		.test = next_token,
		.setup = setup_fort,
		.tear_down = teardown_fort
	},
	{
		.name = "/next_char",
		.test = next_char,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{ 0 }
};

MunitSuite scan = {
	.prefix = "/scan",
	.tests = tests
};
