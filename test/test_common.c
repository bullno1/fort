#include "test_common.h"

void
fort_assert_stack_equal(fort_t* fort1, fort_t* fort2)
{
	fort_int_t stack_size1 = fort_get_stack_size(fort1);
	fort_int_t stack_size2 = fort_get_stack_size(fort2);
	munit_assert_int64(stack_size1, ==, stack_size2);

	for(fort_int_t i = 0; i < stack_size1; ++i)
	{
		fort_cell_type_t type1, type2;
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fort1, i, &type1));
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_get_type(fort2, i, &type2));
		munit_assert_enum(fort_cell_type_t, type1, ==, type2);

		fort_int_t cmp;
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_xmove(fort1, fort2, i));
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_pick(fort2, i + 1));
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_find(fort2, FORT_STRING_REF("=")));
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_execute(fort2));
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_pop_integer(fort2, &cmp));
		munit_assert_int64(1, ==, cmp);
	}
}

void*
setup_fixture(const MunitParameter params[], void* userdata)
{
	(void)params;
	(void)userdata;

	fixture_t* fixture = BK_NEW(bk_default_allocator, fixture_t);

	fort_ctx_config_t fort_ctx_cfg = {
		.allocator = bk_default_allocator
	};
	fort_config_t fort_cfg = {
		.output = bk_stdout,
		.param_stack_size = 1024,
		.local_stack_size = 1024,
		.return_stack_size = 1024
	};

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create_ctx(&fort_ctx_cfg, &fixture->ctx));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(fixture->ctx, &fort_cfg, &fixture->fort1));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(fixture->ctx, &fort_cfg, &fixture->fort2));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_load_builtins(fixture->fort1));
	munit_assert_int64(0, ==, fort_get_stack_size(fixture->fort1));
	munit_assert_int64(0, ==, fort_get_stack_size(fixture->fort2));

	return fixture;
}

void
teardown_fixture(void* fixture_)
{
	fixture_t* fixture = fixture_;
	fort_destroy(fixture->fort1);
	fort_destroy(fixture->fort2);
	fort_destroy_ctx(fixture->ctx);
	bk_free(bk_default_allocator, fixture);
}

void*
setup_fort(const MunitParameter params[], void* userdata)
{
	(void)params;
	(void)userdata;

	fort_ctx_config_t fort_ctx_cfg = {
		.allocator = bk_default_allocator
	};
	fort_config_t fort_cfg = {
		.output = bk_stdout,
		.param_stack_size = 1024,
		.local_stack_size = 1024,
		.return_stack_size = 1024
	};
	fort_ctx_t* ctx;
	fort_t* fort;

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create_ctx(&fort_ctx_cfg, &ctx));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(ctx, &fort_cfg, &fort));

	return fort;
}

void
teardown_fort(void* fixture)
{
	fort_t* fort = fixture;
	fort_ctx_t* ctx = fort_get_ctx(fort);
	fort_destroy(fort);
	fort_destroy_ctx(ctx);
}
