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
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_load_builtins(fixture->fort1));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_load_builtins(fixture->fort2));
	munit_assert_int64(0, ==, fort_stack_size(fixture->fort1));
	munit_assert_int64(0, ==, fort_stack_size(fixture->fort2));

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
simple(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", ": a 42 ; a");

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/simple",
		.test = simple,
		.setup = setup,
		.tear_down = teardown
	},
	{ 0 }
};

MunitSuite compile = {
	.prefix = "/compile",
	.tests = tests
};
