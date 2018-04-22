#include <stdio.h>
#include <stdlib.h>
#include <munit/munit.h>

#define TEST_SUITES(X) \
	X(scan) \
	X(stack) \
	X(strpool) \
	X(interpret) \
	X(stack) \
	X(compile)

#define DECLARE_SUITE(S) extern MunitSuite S;

#define IMPORT_SUITE(S) S,

TEST_SUITES(DECLARE_SUITE)

int
main(int argc, char* argv[])
{
	MunitSuite all_suites[] = {
		TEST_SUITES(IMPORT_SUITE)
		{ 0 }
	};

	MunitSuite main_suite = {
		.prefix = "",
		.suites = all_suites
	};

	return munit_suite_main(&main_suite, NULL, argc, argv);
}
