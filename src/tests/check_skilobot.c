#include <stdlib.h>
#include <check.h>
#include "skilobot.h"

// Needed to compile any program with a library.
//#include "kilolib.h"
typedef struct { } USERDATA;
int UserdataSize = sizeof(USERDATA);
void *mydata;
int bot_main(void) { return 0; };
char* botinfo_simple(void) { return NULL; };
extern uint16_t kilo_uid;

START_TEST(test_new_kilobot)
{
    kilobot* k;
    k = new_kilobot(20, 1);
    ck_assert_int_eq(k->ID, 20);
}
END_TEST

Suite *add_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("skilobot");
    tc_core = tcase_create("core");

    tcase_add_test(tc_core, test_new_kilobot);
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
