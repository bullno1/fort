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

fort_err_t
fort_interpret_string(fort_t* fort, char* string)
{
	bk_mem_file_t mem_file;
	bk_file_t* file = bk_mem_fs_wrap_fixed(&mem_file, string, strlen(string));

	return fort_interpret(fort, file, __FILE__);
}
