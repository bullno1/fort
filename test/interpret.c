#include "test_common.h"

typedef struct
{
	fort_t* fort1;
	fort_t* fort2;
} fixture_t;

#define fort_assert_same_stack_effect(fort1, fort2, str1, str2) \
	do { \
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_interpret_string(fort1, str1)); \
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_interpret_string(fort2, str2)); \
		fort_assert_stack_equal(fort1, fort2); \
	} while(0)

static void*
setup(const MunitParameter params[], void* userdata)
{
	(void)params;
	(void)userdata;

	fixture_t* fixture = BK_NEW(bk_default_allocator, fixture_t);

	fort_config_t fort_cfg = {
		.allocator = bk_default_allocator,
		.output = bk_stdout
	};

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(&fort_cfg, &fixture->fort1));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(&fort_cfg, &fixture->fort2));

	return fixture;
}

static void
teardown(void* fixture_)
{
	fixture_t* fixture = fixture_;
	fort_destroy(fixture->fort1);
	fort_destroy(fixture->fort2);
	bk_free(bk_default_allocator, fixture);
}

static fort_err_t
fort_interpret_string(fort_t* fort, char* string)
{
	bk_mem_file_t mem_file;
	bk_file_t* file = bk_mem_fs_wrap_fixed(&mem_file, string, strlen(string));

	return fort_interpret(fort, file);
}

static void
fort_assert_stack_equal(fort_t* fort1, fort_t* fort2)
{
	fort_int_t stack_size1 = fort_stack_size(fort1);
	fort_int_t stack_size2 = fort_stack_size(fort2);
	munit_assert_int64(stack_size1, ==, stack_size2);

	for(fort_int_t i = 0; i < stack_size1; ++i)
	{
		fort_cell_type_t type1, type2;
		fort_cell_t value1, value2;

		fort_peek(fort1, i, &type1, &value1);
		fort_peek(fort2, i, &type2, &value2);

		munit_assert_enum(fort_cell_type_t, type1, ==, type2);
		munit_assert_memory_equal(sizeof(fort_cell_t), &value1, &value2);
	}
}

static MunitResult
number(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;
	fort_cell_type_t type;
	fort_cell_t value;

	fort_push_integer(fixture->fort1, 42);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_peek(fixture->fort1, 0, &type, &value));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, type);
	munit_assert_int64(42, ==, value.integer);

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_interpret_string(fixture->fort2, "42"));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_peek(fixture->fort1, 0, &type, &value));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, type);
	munit_assert_int64(42, ==, value.integer);

	munit_assert_int64(1, ==, fort_stack_size(fixture->fort1));
	munit_assert_int64(1, ==, fort_stack_size(fixture->fort2));


	fort_assert_stack_equal(fixture->fort1, fixture->fort2);

	fort_reset(fixture->fort1);
	fort_reset(fixture->fort2);
	munit_assert_int64(0, ==, fort_stack_size(fixture->fort1));
	munit_assert_int64(0, ==, fort_stack_size(fixture->fort2));

	fort_push_real(fixture->fort1, -42.5);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_interpret_string(fixture->fort2, "-42.5"));
	fort_assert_stack_equal(fixture->fort1, fixture->fort2);

	return MUNIT_OK;
}

static MunitResult
arithmetic(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", "41 1 +");

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/number",
		.test = number,
		.setup = setup,
		.tear_down = teardown
	},
	{
		.name = "/arithmetic",
		.test = arithmetic,
		.setup = setup,
		.tear_down = teardown
	},
	{ 0 }
};

MunitSuite interpret = {
	.prefix = "/interpret",
	.tests = tests
};
