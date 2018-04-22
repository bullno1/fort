#include "test_common.h"

static MunitResult
push_pop(const MunitParameter params[], void* fixture)
{
	(void)params;
	fort_t* fort = fixture;
	fort_cell_type_t type;
	fort_int_t int_val;
	fort_real_t real_val;

	munit_assert_int64(0, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_integer(fort, 42));
	munit_assert_int64(1, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fort, 0, &type));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, type);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, 0, &int_val));
	munit_assert_int64(42, ==, int_val);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_real(fort, 0, &real_val));
	munit_assert_double(42.0, ==, real_val);

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_real(fort, 42.5));
	munit_assert_int64(2, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fort, 0, &type));
	munit_assert_enum(fort_cell_type_t, FORT_REAL, ==, type);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, 0, &int_val));
	munit_assert_int64(42, ==, int_val);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_real(fort, 0, &real_val));
	munit_assert_double(42.5, ==, real_val);

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_ndrop(fort, 2));
	munit_assert_int64(0, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_ERR_UNDERFLOW, ==, fort_ndrop(fort, 2));

	return MUNIT_OK;
}

static void*
setup_stack(const MunitParameter params[], void* userdata)
{
	fort_t* fort = setup_fort(params, userdata);

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_integer(fort, 1));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_integer(fort, 2));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_integer(fort, 3));

	return fort;
}

static MunitResult
pick(const MunitParameter params[], void* fixture)
{
	(void)params;
	fort_t* fort = fixture;
	fort_cell_type_t type;
	fort_int_t int_val;

	munit_assert_int64(3, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_pick(fort, 1));
	munit_assert_int64(4, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fort, 0, &type));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, type);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_pop_integer(fort, &int_val));
	munit_assert_int64(2, ==, int_val);

	munit_assert_int64(3, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_pick(fort, -1));
	munit_assert_int64(4, ==, fort_get_stack_size(fort));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fort, 0, &type));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, type);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_pop_integer(fort, &int_val));
	munit_assert_int64(1, ==, int_val);

	munit_assert_enum(fort_err_t, FORT_ERR_UNDERFLOW, ==, fort_pick(fort, 3));
	munit_assert_enum(fort_err_t, FORT_ERR_OVERFLOW, ==, fort_pick(fort, -4));

	return MUNIT_OK;
}

static MunitResult
roll(const MunitParameter params[], void* fixture)
{
	(void)params;
	fort_t* fort = fixture;
	fort_int_t int_val;

	// 1 2 3
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_roll(fort, 2));
	// 2 3 1
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, -1, &int_val));
	munit_assert_int64(2, ==, int_val);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, -2, &int_val));
	munit_assert_int64(3, ==, int_val);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, -3, &int_val));
	munit_assert_int64(1, ==, int_val);

	munit_assert_enum(fort_err_t, FORT_ERR_UNDERFLOW, ==, fort_roll(fort, 3));

	return MUNIT_OK;
}

static MunitResult
move(const MunitParameter params[], void* fixture)
{
	(void)params;
	fort_t* fort = fixture;
	fort_int_t int_val;

	// 1 2 3
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_move(fort, 1, 0));
	// 1 2 2
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, -1, &int_val));
	munit_assert_int64(1, ==, int_val);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, -2, &int_val));
	munit_assert_int64(2, ==, int_val);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fort, -3, &int_val));
	munit_assert_int64(2, ==, int_val);

	munit_assert_enum(fort_err_t, FORT_ERR_UNDERFLOW, ==, fort_move(fort, 1, 3));

	return MUNIT_OK;
}

static MunitResult
xmove(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;
	fort_t* fort1 = fixture->fort1;
	fort_t* fort2 = fixture->fort2;

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_integer(fort1, 1));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_integer(fort1, 2));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_push_integer(fort1, 3));

	fort_int_t int_val;
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_xmove(fort1, fort2, 2));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_pop_integer(fort2, &int_val));
	munit_assert_int64(1, ==, int_val);

	munit_assert_enum(fort_err_t, FORT_ERR_UNDERFLOW, ==, fort_xmove(fort1, fort2, 3));
	munit_assert_enum(fort_err_t, FORT_ERR_OVERFLOW, ==, fort_xmove(fort1, fort2, -4));

	return MUNIT_OK;
}

static MunitResult
fort(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "1 2", "1 2 3 1 ndrop");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "1 2 3 2", "1 2 3 1 pick");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "2 3 1", "1 2 3 2 roll");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "3 3", "3 dup");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "1 2 1", "1 2 over");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "2 1", "1 2 swap");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "2 3 1", "1 2 3 rot");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "1 2", "1 2 3 drop");

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/push_pop",
		.test = push_pop,
		.setup = setup_fort,
		.tear_down = teardown_fort
	},
	{
		.name = "/pick",
		.test = pick,
		.setup = setup_stack,
		.tear_down = teardown_fort
	},
	{
		.name = "/roll",
		.test = roll,
		.setup = setup_stack,
		.tear_down = teardown_fort
	},
	{
		.name = "/move",
		.test = move,
		.setup = setup_stack,
		.tear_down = teardown_fort
	},
	{
		.name = "/xmove",
		.test = xmove,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{
		.name = "/fort",
		.test = fort,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{ 0 }
};

MunitSuite stack = {
	.prefix = "/stack",
	.tests = tests
};
