#include<stdlib.h>
#include<stdint.h>

#ifndef SKILOBOT_H
#define SKILOBOT_H

//#include "userdata.h" //declarations of user data

#define MAXCOMMLINES 10000

typedef struct {
  double x, y;
  double *x_history, *y_history;
  int p_hist, n_hist;
  
  int right_motor_power, left_motor_power;
  int ID;
  double direction; //Angle relative to constant x, +ve y in radians
  int r_led, g_led, b_led;
  int radius;

  int *in_range;
  int n_in_range;

  /* Messaging */
  double cr; // Communication radius
  int tx_enabled;  //1 if the bot is transmitting - used for drawing communication circles
  int tx_ticks;    //the time in ticks when this bot is to transmit next
  
  int screen_x, screen_y; //where the bot is drawn on screen
  
  uint8_t seed;  //for the software random number generator
  uint8_t accumulator;

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
extern void (*user_setup)(void);
extern void (*user_loop)(void);

extern kilobot** allbots;
void create_bots(int n_bots);
kilobot *new_kilobot(int ID, int n_bots);
void init_all_bots(int n_bots);
void run_all_bots(int n_bots);
void dump_all_bots(int n_bots);
void update_all_bots(int n_bots, float timestep);
void spread_out(double k);

void update_interactions(int n_bots);

extern int current_bot;
extern void* mydata;

kilobot *Me();

int bot_main (void);

enum {PAUSE, RUNNING};
extern int state;   //simulator state. PAUSE or RUNNING.
extern int tx_period_ticks;
extern int fullSpeed;
extern int stepsPerFrame;

#endif
