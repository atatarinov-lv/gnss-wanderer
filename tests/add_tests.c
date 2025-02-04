#include "minunit.h"
#include <stdio.h>
#include <example_add.h>

char *test_add()
{
    mu_assert(add(1) == 2, "the result must be equal 2");

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_add);

    return NULL;
}

RUN_TESTS(all_tests);
