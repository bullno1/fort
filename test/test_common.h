#ifndef FORT_TEST_COMMON_H
#define FORT_TEST_COMMON_H

#include <munit/munit.h>
#include <fort.h>
#include <bk/stdstream.h>
#include <bk/fs/mem.h>
#include <bk/allocator.h>
#include <bk/default_allocator.h>

#define munit_assert_enum(T, a, op, b) \
  do { \
    T munit_tmp_a_ = (a); \
    T munit_tmp_b_ = (b); \
    if (!(munit_tmp_a_ op munit_tmp_b_)) {                               \
      munit_errorf("assertion failed: " #a " " #op " " #b " (%d (%s) " #op " %d (%s))", \
                   munit_tmp_a_, BK_CONCAT(T, _to_str)(munit_tmp_a_), \
				   munit_tmp_b_, BK_CONCAT(T, _to_str)(munit_tmp_b_)); \
    } \
  } while (0)

#define BK_CONCAT(A, B) BK_CONCAT2(A, B)
#define BK_CONCAT2(A, B) A##B

#define fort_assert_same_stack_effect(fort1, fort2, str1, str2) \
	do { \
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_interpret_string(fort1, str1)); \
		munit_assert_enum(fort_err_t, FORT_OK, ==, fort_interpret_string(fort2, str2)); \
		fort_assert_stack_equal(fort1, fort2); \
	} while(0)

void
fort_assert_stack_equal(fort_t* fort1, fort_t* fort2);

fort_err_t
fort_interpret_string(fort_t* fort, char* string);

#endif
