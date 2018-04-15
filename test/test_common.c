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
