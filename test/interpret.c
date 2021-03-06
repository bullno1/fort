#include "test_common.h"

static MunitResult
number(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;
	fort_cell_type_t type;
	fort_int_t int_val;
	fort_real_t real_val;

	munit_assert_enum(
		fort_err_t,
		FORT_OK,
		==,
		fort_interpret_string(
			fixture->fort1,
			FORT_STRING_REF("42"),
			FORT_STRING_REF(__FILE__)
		)
	);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fixture->fort1, 0, &type));
	munit_assert_enum(fort_cell_type_t, FORT_INTEGER, ==, type);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_integer(fixture->fort1, 0, &int_val));
	munit_assert_int64(42, ==, int_val);

	munit_assert_enum(
		fort_err_t,
		FORT_OK,
		==,
		fort_interpret_string(
			fixture->fort1,
			FORT_STRING_REF("-42.5"),
			FORT_STRING_REF(__FILE__)
		)
	);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fixture->fort1, 0, &type));
	munit_assert_enum(fort_cell_type_t, FORT_REAL, ==, type);
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_real(fixture->fort1, 0, &real_val));
	munit_assert_double(-42.5, ==, real_val);

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

static MunitResult
safe_exec(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	munit_assert_enum(
		fort_err_t,
		FORT_ERR_INVALID, ==,
		fort_interpret_string(
			fixture->fort1,
			FORT_STRING_REF("word.create word.->colon execute"),
			FORT_STRING_REF(__FILE__)
		)
	);

	return MUNIT_OK;
}

static MunitResult
execute(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42 3", "41 ' 1+ execute 3");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42 3", "41 : add  1 + ; ' add execute 3");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42 3", "41 : add  1 quote> + execute ; add 3");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42 3", "41 : add  1 quote> + execute 3 ; ' add execute");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42 3", "41 : add no-inline 1 quote> + execute 3 ; : add2 add ; add2");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42 3", "41 : add no-inline 1 quote> + execute 3 ; : add2 add ; ' add2 execute");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42 3", "41 : add no-inline 1 quote> + execute 3 ; : add2 quote> add quote> execute execute ; ' add2 execute");

	return MUNIT_OK;
}

static MunitResult
eval(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", "\" 42\" eval");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", ": hipster+ \" 1 + \" eval ; 41 hipster+");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", ": hipster+ \" 1 + \" eval ; 41 \" ' hipster+ execute\" eval");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", ": hipster+ \" 1 + \" eval ; : hipster+2 \" ' hipster+\" eval execute ; 41 hipster+2");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", ": hipster+ \" 1 + \" eval ; : hipster+2 \" ' hipster+ execute\" eval ; 41 hipster+2");
	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "42", ": hipster+ \" 1 + \" eval ; : hipster+2 \" ' hipster+ execute\" eval ; 41 \" ' hipster+2  execute \" eval");

	return MUNIT_OK;
}

static MunitResult
string(const MunitParameter params[], void* fixture_)
{
	(void)params;
	fixture_t* fixture = fixture_;

	munit_assert_enum(
		fort_err_t,
		FORT_OK, ==,
		fort_interpret_string(
			fixture->fort1,
			FORT_STRING_REF("\" hello world\""),
			FORT_STRING_REF(__FILE__)
		)
	);

	fort_string_ref_t str;
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_string(fixture->fort1, 0, &str));
	munit_assert_string_equal("hello world", str.ptr);

	munit_assert_enum(
		fort_err_t,
		FORT_OK, ==,
		fort_interpret_string(
			fixture->fort1,
			FORT_STRING_REF("\" hello world \""),
			FORT_STRING_REF(__FILE__)
		)
	);

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_as_string(fixture->fort1, 0, &str));
	munit_assert_string_equal("hello world ", str.ptr);

	fort_assert_same_stack_effect(fixture->fort1, fixture->fort2, "\" test\"", ": test \" test\" ; test");

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/number",
		.test = number,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{
		.name = "/string",
		.test = string,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{
		.name = "/arithmetic",
		.test = arithmetic,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{
		.name = "/execute",
		.test = execute,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{
		.name = "/eval",
		.test = eval,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{
		.name = "/safe_exec",
		.test = safe_exec,
		.setup = setup_fixture,
		.tear_down = teardown_fixture
	},
	{ 0 }
};

MunitSuite interpret = {
	.prefix = "/interpret",
	.tests = tests
};
