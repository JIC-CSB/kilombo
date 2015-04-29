#include <stdlib.h>
#include <check.h>
#include "skilobot.h"

#include <stdio.h>

// Needed to compile any program with a library.
//#include "kilolib.h"
typedef struct { int num_bot_steps; } USERDATA;
int UserdataSize = sizeof(USERDATA);
void *mydata;
char* botinfo_simple(void) { return NULL; };
extern uint16_t kilo_uid;

// Define a dummy bot_main function for testing purposes.
int n_calls_to_bot_main = 0;
int bot_main(void) {
    n_calls_to_bot_main++;
    return 0;
};

// Define dummy setup and loop functions for testing purposes.
void setup(void) {
    ((USERDATA* )mydata)->num_bot_steps = 0;
}
void dummy_loop(void) {
    ((USERDATA* )mydata)->num_bot_steps++;
}


START_TEST(test_new_kilobot)
{
    kilobot* k;
    k = new_kilobot(20, 1);
    ck_assert_int_eq(k->ID, 20);
}
END_TEST

START_TEST(test_create_bots)
{
    int n = 3;
    create_bots(n);
    for (int i=0; i<n; i++) {
        ck_assert_int_eq(allbots[i]->ID, i);
    }
}
END_TEST

START_TEST(test_init_all_bots)
{
    int n = 3;
    n_calls_to_bot_main = 0;  // Clean up from any previous tests.
    create_bots(n);
    init_all_bots(n);
    ck_assert_int_eq(n_calls_to_bot_main, n);
}
END_TEST

START_TEST(test_me)
{
    int n = 3;
    create_bots(n);
    for (int i=0; i<n; i++) {
        current_bot = i;
        ck_assert_int_eq(Me()->ID, i);
    }
}
END_TEST

START_TEST(test_run_all_bots)
{
    int n = 3;
    USERDATA* d;
    create_bots(n);
    init_all_bots(n);
    for (int i=0; i<n; i++) {
        current_bot = i;
        mydata = (USERDATA* )(Me()->data);
        setup();
    }
    user_loop = &dummy_loop;
    run_all_bots(n);
    for (int i=0; i<n; i++) {
        current_bot = i;
        d = ((USERDATA* )Me()->data);
        ck_assert_int_eq(d->num_bot_steps, 1);
    }
}
END_TEST

Suite *add_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("skilobot");
    tc_core = tcase_create("core");

    tcase_add_test(tc_core, test_new_kilobot);
    tcase_add_test(tc_core, test_create_bots);
    tcase_add_test(tc_core, test_init_all_bots);
    tcase_add_test(tc_core, test_me);
    tcase_add_test(tc_core, test_run_all_bots);
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
