#include "test_common.h"
#include "../src/internal.h"

static MunitResult
basic(const MunitParameter params[], void* fixture)
{
	(void)params;
	fort_t* fort = fixture;
	fort_string_t* str = NULL;
	fort_string_t* str2 = NULL;
	fort_string_t* str3 = NULL;

	munit_assert_enum(
		fort_err_t,
		FORT_ERR_NOT_FOUND,
		==,
		fort_strpool_check(fort->ctx, FORT_STRING_REF(":"), &str)
	);

	munit_assert_enum(
		fort_err_t,
		FORT_OK,
		==,
		fort_strpool_alloc(fort->ctx, FORT_STRING_REF(":"), &str)
	);
	munit_assert_string_equal(":", str->ptr);

	munit_assert_enum(
		fort_err_t,
		FORT_OK,
		==,
		fort_strpool_check(fort->ctx, FORT_STRING_REF(":"), &str2)
	);
	munit_assert_ptr(str, ==, str2);

	munit_assert_enum(
		fort_err_t,
		FORT_OK,
		==,
		fort_strpool_alloc(fort->ctx, FORT_STRING_REF(":"), &str2)
	);
	munit_assert_ptr(str, ==, str2);

	munit_assert_enum(
		fort_err_t,
		FORT_ERR_NOT_FOUND,
		==,
		fort_strpool_check(fort->ctx, FORT_STRING_REF("wat"), &str3)
	);
	munit_assert_enum(
		fort_err_t,
		FORT_OK,
		==,
		fort_strpool_alloc(fort->ctx, FORT_STRING_REF("wat"), &str3)
	);
	munit_assert_ptr(str, !=, str3);

	munit_assert_enum(
		fort_err_t,
		FORT_OK,
		==,
		fort_strpool_release(fort->ctx, str)
	);
	munit_assert_enum(
		fort_err_t,
		FORT_ERR_NOT_FOUND,
		==,
		fort_strpool_check(fort->ctx, FORT_STRING_REF(":"), &str)
	);

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/basic",
		.test = basic,
		.setup = setup_fort,
		.tear_down = teardown_fort
	},
	{ 0 }
};

MunitSuite strpool = {
	.prefix = "/strpool",
	.tests = tests
};
