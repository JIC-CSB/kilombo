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


// Helper function to do a double comparison.

void check_double_equality(double d1, double d2) {
    double abs_diff = abs(d1 - d2);
    fail_unless(abs_diff < 0.00001, "double not equal");
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

START_TEST(test_update_history)
{
    kilobot* k;
    k = new_kilobot(0, 1);

    ck_assert_int_eq(k->x, 0);
    ck_assert_int_eq(k->y, 0);

    update_history(k);

    ck_assert_int_eq(k->x_history[0], 0);
    ck_assert_int_eq(k->y_history[0], 0);


    k->x = 1;
    k->y = 2;
    update_history(k);

    ck_assert_int_eq(k->x_history[1], 1);
    ck_assert_int_eq(k->y_history[1], 2);
}
END_TEST

START_TEST(test_bot_dist)
{
    kilobot* k1;
    kilobot* k2;
    k1 = new_kilobot(0, 1);
    k2 = new_kilobot(0, 1);
    k1->x = 0.0;
    k1->y = 0.0;
    k2->x = 3.0;
    k2->y = 4.0;
    double distance = bot_dist(k1, k2);
    check_double_equality(distance, 5.0);
//    double abs_diff = abs(distance - 5.0);
//    fail_unless(abs_diff < 0.001, "Distance is incorrect.");
}
END_TEST

START_TEST(test_normalise)
{
    coord2D c;
    c.x = 0.0;
    c.y = 0.0;
    c = normalise(c);

    check_double_equality(c.x, 1.0);
    check_double_equality(c.y, 0.0);

    c.x = 5.0;
    c.y = 5.0;
    c = normalise(c);
    check_double_equality(c.x, 1.4142135);
    check_double_equality(c.y, 1.4142135);
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
    tcase_add_test(tc_core, test_update_history);
    tcase_add_test(tc_core, test_bot_dist);
    tcase_add_test(tc_core, test_normalise);
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
