#include "test_common.h"

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
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{ 0 }
};

MunitSuite compile = {
	.prefix = "/compile",
	.tests = tests
};
