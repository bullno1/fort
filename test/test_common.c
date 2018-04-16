#include "test_common.h"

void
fort_assert_stack_equal(fort_t* fort1, fort_t* fort2)
{
	fort_int_t stack_size1 = fort_stack_size(fort1);
	fort_int_t stack_size2 = fort_stack_size(fort2);
	munit_assert_int64(stack_size1, ==, stack_size2);

	for(fort_int_t i = 0; i < stack_size1; ++i)
	{
		fort_cell_t value1, value2;

		fort_peek(fort1, i, &value1);
		fort_peek(fort2, i, &value2);

		munit_assert_enum(fort_cell_type_t, value1.type, ==, value2.type);
		munit_assert_memory_equal(sizeof(fort_cell_t), &value1, &value2);
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
		.output = bk_stdout
	};

	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create_ctx(&fort_ctx_cfg, &fixture->ctx));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(fixture->ctx, &fort_cfg, &fixture->fort1));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_create(fixture->ctx, &fort_cfg, &fixture->fort2));
	munit_assert_enum(fort_err_t, FORT_OK, ==, fort_load_builtins(fixture->fort1));
	munit_assert_int64(0, ==, fort_stack_size(fixture->fort1));
	munit_assert_int64(0, ==, fort_stack_size(fixture->fort2));

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
