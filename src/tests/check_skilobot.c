#include <stdlib.h>
#include <check.h>
#include "skilobot.h"

START_TEST(test_framework_working)
{
    ck_assert_int_eq(1, 1);
}
END_TEST

Suite *add_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("skilobot");
    tc_core = tcase_create("core");

    tcase_add_test(tc_core, test_framework_working);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = add_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
