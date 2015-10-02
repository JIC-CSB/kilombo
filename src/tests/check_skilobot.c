#include <stdlib.h>
#include <check.h>

#include <stdio.h>
#include "skilobot.h"
#undef main // to prevent main here from being re-defined

#include "params.h"



// Include some definitions of "private" functions we want to test.
void update_bot_history(kilobot *bot);
void manage_bot_history_memory(kilobot *bot);
void move_bot_forward(kilobot *bot, float timestep);
void turn_bot_right(kilobot *bot, float timestep);
void turn_bot_left(kilobot *bot, float timestep);
void update_bot_location(kilobot *bot, float timestep);
double bot_dist(kilobot *bot1, kilobot *bot2);
coord2D normalise(coord2D c);
coord2D separation_unit_vector(kilobot *bot1, kilobot *bot2);
void separate_clashing_bots(kilobot *bot1, kilobot *bot2);
void reset_n_in_range_indices(int n_bots);
void update_n_in_range_indices(kilobot *bot1, kilobot *bot2);

// Needed to compile any program with a library.
//#include "kilolib.h"
typedef struct { int num_bot_steps; } USERDATA;
int UserdataSize = sizeof(USERDATA);
void *mydata;
//char* botinfo_simple(void) { return NULL; }; 
extern uint16_t kilo_uid;

// simulator parameter structure.
// to avoid dragging in the whole parameter parsing in this test, populate with default values here as needed.
simulation_params params = {
  .commsRadius = 70
};

simulation_params* simparams = &params;


// Needed to setup a new kilobot.
int get_int_param(const char *param_name, int default_val) { return default_val; };

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
      prepare_bot(allbots[i]);
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
      prepare_bot(allbots[i]);
      mydata = (USERDATA* )(Me()->data);
      current_bot->user_loop = &dummy_loop;
      setup();
    }
    
    run_all_bots(n);
    for (int i=0; i<n; i++) {
      prepare_bot(allbots[i]);
      d = ((USERDATA* )Me()->data);
      ck_assert_int_eq(d->num_bot_steps, 1);
    }
}
END_TEST

START_TEST(test_update_bot_history)
{
    kilobot* k;
    k = new_kilobot(0, 1);

    ck_assert_int_eq(k->x, 0);
    ck_assert_int_eq(k->y, 0);

    update_bot_history(k);

    ck_assert_int_eq(k->x_history[0], 0);
    ck_assert_int_eq(k->y_history[0], 0);


    k->x = 1;
    k->y = 2;
    update_bot_history(k);

    ck_assert_int_eq(k->x_history[1], 1);
    ck_assert_int_eq(k->y_history[1], 2);
}
END_TEST

START_TEST(test_manage_bot_history_memory)
{
  /*
    kilobot* k;
    k = new_kilobot(0, 1);

    ck_assert_int_eq(k->n_hist, 100);
    
    k->p_hist = 100;
    manage_bot_history_memory(k);

    ck_assert_int_eq(k->n_hist, 200);

    This test no longer passes, as the mechanism changed.
    Now, the history does not grow forever, but is limited
    to the histLength latest points.
  */
}
END_TEST

START_TEST(test_move_bot_forward)
{
    kilobot* k;
    k = new_kilobot(0, 1);

    // Move forwards along the north axis.
    k->right_motor_power = 2;
    k->left_motor_power = 2;
    k->direction = 0.0;
    move_bot_forward(k, 3.0);

    check_double_equality(k->x, 0.0);
    check_double_equality(k->y, 6.0);
}
END_TEST

START_TEST(test_turn_bot_right)
{
    kilobot* k;
    k = new_kilobot(0, 1);

    // Turn from North to East.
    k->direction = 0.0;
    k->left_motor_power = 30;
    turn_bot_right(k, 3.1415927/2);

    check_double_equality( k->direction, 3.1415927/2 );

    // this test assumes leg is placed at 90 degrees, no longer true
    // check_double_equality(k->x, 17.0);
    // check_double_equality(k->y, 17.0);
}
END_TEST

START_TEST(test_turn_bot_left)
{
    kilobot* k;
    k = new_kilobot(0, 1);

    // Turn from North to West.
    k->direction = 0.0;
    k->right_motor_power = 30;
    turn_bot_left(k, 3.1415927/2);

    check_double_equality( k->direction, -3.1415927/2 );

    // this test assumes leg is placed at 90 degrees, no longer true
    // check_double_equality(k->x, -17.0);
    // check_double_equality(k->y, 17.0);
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

START_TEST(test_separation_unit_vector)
{
    kilobot* k1;
    kilobot* k2;
    coord2D c;

    k1 = new_kilobot(0, 1);
    k2 = new_kilobot(0, 1);
    k1->x = 0.0;
    k1->y = 4.0;
    k2->x = 7.6;
    k2->y = 4.0;

    c = separation_unit_vector(k1, k2);

    check_double_equality(c.x, 1.0);
    check_double_equality(c.y, 0.0);

}
END_TEST

START_TEST(test_separate_clashing_bots)
{
    kilobot* k1;
    kilobot* k2;

    k1 = new_kilobot(0, 1);
    k2 = new_kilobot(0, 1);
    k1->x = 4.0;
    k1->y = 4.0;
    k2->x = 4.0;
    k2->y = 10.0;

    separate_clashing_bots(k1, k2);

    check_double_equality(k1->x, 4.0);
    check_double_equality(k1->y, 3.0);
    check_double_equality(k2->x, 4.0);
    check_double_equality(k2->y, 11.0);

}
END_TEST

START_TEST(test_reset_n_in_range_indices)
{
    int n = 3;
    create_bots(n);
    init_all_bots(n);
    for (int i=0; i<n; i++) {
        allbots[i]->n_in_range = i;
        ck_assert_int_eq(allbots[i]->n_in_range, i);
    }

    reset_n_in_range_indices(n);
    for (int i=0; i<n; i++) {
        ck_assert_int_eq(allbots[i]->n_in_range, 0);
    }
}
END_TEST

START_TEST(test_update_n_in_range_indices)
{
    // Setup.
    int n = 2;
    create_bots(n);
    init_all_bots(n);
    reset_n_in_range_indices(n);

    // Code we want to test.
    update_n_in_range_indices(allbots[0], allbots[1]);

    // Both bots have each other in range.
    ck_assert_int_eq(allbots[0]->in_range[0], 1);
    ck_assert_int_eq(allbots[1]->in_range[0], 0);

    // The n_in_range index is ready to add the next bot.
    for (int i=0; i<n; i++) {
        ck_assert_int_eq(allbots[i]->n_in_range, 1);
    }

}
END_TEST

START_TEST(test_update_interactions)
{
    // Setup.
    int n = 2;
    create_bots(n);
    init_all_bots(n);
    reset_n_in_range_indices(n);
    for (int i=0; i<n; i++) {
        allbots[i]->cr = 50;
        allbots[i]->radius = 20;
    }
    allbots[0]->x = 0.0;
    allbots[0]->y = 0.0;

    // Not within communication distance.
    allbots[1]->x = 0.0;
    allbots[1]->y = 50.0;
    update_interactions(n);
    for (int i=0; i<n; i++) {
        ck_assert_int_eq(allbots[i]->n_in_range, 0);
    }
    check_double_equality(allbots[0]->x, 0.0);
    check_double_equality(allbots[0]->y, 0.0);
    check_double_equality(allbots[1]->x, 0.0);
    check_double_equality(allbots[1]->y, 50.0);

    // Just within communication distance.
    allbots[1]->x = 0.0;
    allbots[1]->y = 49.9;
    update_interactions(n);
    for (int i=0; i<n; i++) {
        ck_assert_int_eq(allbots[i]->n_in_range, 1);
    }
    check_double_equality(allbots[0]->x, 0.0);
    check_double_equality(allbots[0]->y, 0.0);
    check_double_equality(allbots[1]->x, 0.0);
    check_double_equality(allbots[1]->y, 49.9);

    // Touching but not collided.
    allbots[1]->x = 0.0;
    allbots[1]->y = 40.0;
    update_interactions(n);
    for (int i=0; i<n; i++) {
        ck_assert_int_eq(allbots[i]->n_in_range, 1);
    }
    check_double_equality(allbots[0]->x, 0.0);
    check_double_equality(allbots[0]->y, 0.0);
    check_double_equality(allbots[1]->x, 0.0);
    check_double_equality(allbots[1]->y, 40.0);

    // Collided.
    allbots[1]->x = 0.0;
    allbots[1]->y = 39.9;
    update_interactions(n);
    for (int i=0; i<n; i++) {
        ck_assert_int_eq(allbots[i]->n_in_range, 1);
    }
    check_double_equality(allbots[0]->x, 0.0);
    check_double_equality(allbots[0]->y, -1.0);
    check_double_equality(allbots[1]->x, 0.0);
    check_double_equality(allbots[1]->y, 40.9);
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
    tcase_add_test(tc_core, test_update_bot_history);
    tcase_add_test(tc_core, test_manage_bot_history_memory);
    tcase_add_test(tc_core, test_move_bot_forward);
    tcase_add_test(tc_core, test_turn_bot_right);
    tcase_add_test(tc_core, test_turn_bot_left);
    tcase_add_test(tc_core, test_bot_dist);
    tcase_add_test(tc_core, test_normalise);
    tcase_add_test(tc_core, test_separation_unit_vector);
    tcase_add_test(tc_core, test_separate_clashing_bots);
    tcase_add_test(tc_core, test_reset_n_in_range_indices);
    tcase_add_test(tc_core, test_update_n_in_range_indices);
    tcase_add_test(tc_core, test_update_interactions);
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

    //srunner_run_all(sr, CK_NORMAL);
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
