#include "test_common.h"

typedef struct
{
	fort_t* fort1;
	fort_t* fort2;
} fixture_t;

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

static MunitResult
number(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;
	fort_cell_t value;

	fort_push_integer(fixture->fort1, 42);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_peek(fixture->fort1, 0, &value));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, value.type);
	munit_assert_int64(42, ==, value.data.integer);

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_interpret_string(fixture->fort2, "42"));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_peek(fixture->fort1, 0, &value));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, value.type);
	munit_assert_int64(42, ==, value.data.integer);

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
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42.0", "41 1.0 +");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42.0", "41.0 1 +");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42.0", "41.0 1.0 +");

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
