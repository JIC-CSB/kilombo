#include<stdlib.h>
#include<stdint.h>
#include"kilolib.h"

#ifndef SKILOBOT_H
#define SKILOBOT_H


#ifndef M_PI
// C99 doesn't define M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

//#include "userdata.h" //declarations of user data

#define MAXCOMMLINES 10000

typedef struct {
  double x, y;
  double *x_history, *y_history;
  int p_hist; // current index in history (ring) buffer
  int n_hist; // size of the history ring buffer 
  int l_hist; // number of history points stored
  
  int right_motor_power, left_motor_power;
  double speed;                     // speed in mm / s
  double turn_rate_l, turn_rate_r;  // turning rate right and left, radians / s
  int ID;
  double direction; // Angle relative to constant x, +ve y in radians
  int r_led, g_led, b_led;
  int radius;       // kilobot radius in mm
  double leg_angle; // angle front leg - center - rear leg in radians

  int *in_range;
  int n_in_range;

  /* Messaging */
  double cr; // Communication radius
  int tx_enabled;  //1 if the bot is transmitting - used for drawing communication circles
  int tx_ticks;    //the time in ticks when this bot is to transmit next
  
  int screen_x, screen_y; //where the bot is drawn on screen

  /* Random number generator */ 
  uint8_t seed;  //for the software random number generator
  uint8_t accumulator;

  /* Setup and loop functions */
  void (*user_setup)(void);
  void (*user_loop)(void);

  // 
  message_tx_t kilo_message_tx;
  message_tx_success_t kilo_message_tx_success;
  message_rx_t kilo_message_rx;
  
  void *data;
  
} kilobot;

typedef struct {
  double x;
  double y;
} coord2D;

typedef struct 
{
  kilobot *from;
  kilobot *to;
  int time;
} CommLine;

extern CommLine commLines[];
extern int NcommLines;


extern int n_bots;
//extern void (*user_setup)(void);
//extern void (*user_loop)(void);

extern kilobot** allbots;
void create_bots(int n_bots);
kilobot *new_kilobot(int ID, int n_bots);
void init_all_bots(int n_bots);
void user_setup_all_bots(int n_bots);
void run_all_bots(int n_bots);
void dump_all_bots(int n_bots);
void update_all_bots(int n_bots, float timestep);
double bot_dist(kilobot *bot1, kilobot *bot2);
void process_bots(int n_bots, float timestep);
void update_interactions(int n_bots);
coord2D separation_unit_vector(kilobot* bot1, kilobot* bot2);
void separate_clashing_bots(kilobot* bot1, kilobot* bot2);
void spread_out(int n_bots, double k);

extern kilobot* current_bot;

// we need to supress this declaration in user code
#ifndef KILOMBO_H
extern void* mydata;
#endif

kilobot *Me();
void prepare_bot(kilobot *bot);
void finalize_bot(kilobot *bot);

int bot_main (void);

enum {PAUSE, RUNNING};
extern int state;   //simulator state. PAUSE or RUNNING.
extern int tx_period_ticks;
extern int fullSpeed;
extern int stepsPerFrame;

extern int16_t (*user_obstacles)(double, double, double *, double *);
extern int16_t (*user_light)(double, double);

#endif
