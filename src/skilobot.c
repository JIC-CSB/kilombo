#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#include "skilobot.h"
#include "kilolib.h"

#include <jansson.h>

kilobot** allbots;
extern int UserdataSize ;  // the size (in bytes) of the USERDATA structure.
                           // filled in by the user program.

int current_bot;

int tx_period_ticks = 15;  // message twice a second

char *botinfo_simple();            // default bot_info function, only returns ID
void (*callback_F5) (void) = NULL; // function pointer to user-defined callback function
                                   // this function is called when F5 is pressed
void (*callback_F6) (void) = NULL; // for F6
char* (*callback_botinfo) (void) = botinfo_simple; // function for bot info, returns a string 
json_t* (*callback_json_state) (void) = NULL; //callback for saving the bot's internal state as JSON


void register_callback(Callback_t type, void (*fp)(void))
{
  switch (type)
    {
    case CALLBACK_PARAMS:
      callback_F5 = fp;
      break;
    case CALLBACK_RESET:
      callback_F6 = fp;
      break;
    case CALLBACK_BOTINFO:
      callback_botinfo = (char*(*)(void)) fp;
      break;
    case CALLBACK_JSON_STATE:
      callback_json_state = (json_t*(*)(void)) fp;
      break;
    }
}

int storeHistory = 1;

typedef struct {
  double x;
  double y;
} coord2D;

kilobot *new_kilobot(int ID, int n_bots)
{
  kilobot* bot = (kilobot*) calloc(1, sizeof(kilobot));
  //calloc sets the memory area to 0 - guarantees initialization of user data

  bot->ID = ID;
  bot->x = 0;
  bot->y = 0;

  bot->x_history = (double *) calloc(100, sizeof(double));
  bot->y_history = (double *) calloc(100, sizeof(double));
  bot->p_hist = 0;
  bot->n_hist = 100;

  bot->radius = 17; // mm

  bot->cwm = 0;
  bot->ccwm = 0;
  bot->direction = (2 * 3.1415927 / 4);
  bot->r_led = 0;
  bot->g_led = 0;
  bot->b_led = 0;

  bot->cr = 70;

  bot->in_range = (int*) malloc(sizeof(int) * n_bots);
  bot->n_in_range = 0;

  bot->tx_ticks = rand() % tx_period_ticks;
  
  bot->data = malloc(UserdataSize);
  
  return bot;
}

void init_all_bots(int n_bots)
{
  /* Initialise the global allbots array */
  allbots = (kilobot**) malloc(sizeof(kilobot*) * n_bots);

  for (int i=0; i<n_bots; i++) {
    allbots[i] = new_kilobot(i, n_bots);

    current_bot = i;      // for Me() to return the right bot
    mydata = Me()->data;
    kilo_uid = i;         // in case the bot's main() uses ID
    bot_main();
  }

}

kilobot *Me()
{
  /* Return the current bot to the calling function, using the global variable
     current_bot to work out which bot is active */

  return allbots[current_bot];
}

void run_all_bots(int n_bots)
{
  /* Run the user program for each bot */
  for (current_bot=0; current_bot<n_bots; current_bot++) {
    //me = & (allbots[current_bot]->data);

    //  alternative way for each bot to access its own data - less copying - FJ
    mydata = allbots[current_bot]->data;
    kilo_uid = allbots[current_bot]->ID;
    //printf ("running bot %d ID: %d\n", current_bot, kilo_uid);
    user_loop();
  }
}

void dump_bot_info(kilobot *self)
{
  /* Dump bot info to stdout */

  printf("B %d: At (%f, %f), speed (%d, %d)\n", 
	 self->ID, self->x, self->y, self->cwm, self->ccwm);
}

void dump_all_bots(int n_bots)
{
  /* Dump info for all bots. Should only be called in a critical region */

  for (int i=0; i<n_bots; i++) {
    dump_bot_info(allbots[i]);
  }
}

void update_bot(kilobot *bot, float timestep)
{
  int r = bot->radius;
  
  double x_l = bot->x - r * cos(bot->direction);
  double y_l = bot->y + r * sin(bot->direction);
  double x_r = bot->x + r * cos(bot->direction);
  double y_r = bot->y - r * sin(bot->direction);

  if (storeHistory)
    {
      bot->x_history[bot->p_hist] = bot->x;
      bot->y_history[bot->p_hist] = bot->y;
      bot->p_hist++;
    }
  
  /* If our history is longer than the memory allocation, reallocate */
  if (1 + bot->p_hist > bot->n_hist) {
    bot->n_hist += 100;
    bot->x_history = (double *) realloc(bot->x_history, sizeof(double) * bot->n_hist);
    bot->y_history = (double *) realloc(bot->y_history, sizeof(double) * bot->n_hist);
  }

  if (bot->cwm && bot->ccwm) { // forward movement

    int velocity = 0.5 * (bot->ccwm + bot->cwm);
    bot->y += timestep * velocity * cos(bot->direction);
    bot->x += timestep * velocity * sin(bot->direction);
  } else {
    if (bot->ccwm) {
      bot->direction += timestep * (double) (bot->ccwm) / 30;
      bot->x = x_r - r * cos(bot->direction);
      bot->y = y_r + r * sin(bot->direction);
    }
    if (bot->cwm) {
      bot->direction -= timestep * (double) (bot->cwm) / 30;
      bot->x = x_l + r * cos(bot->direction);
      bot->y = y_l - r * sin(bot->direction);
    }
  }
}

double bot_dist(kilobot *bot1, kilobot *bot2)
{
  double x1 = bot1->x;
  double x2 = bot2->x;
  double y1 = bot1->y;
  double y2 = bot2->y;

  return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}
// treat c as a vector, return a unit vector parallel to c.
// if c is 0 or very short, special case: choose o.x=1, o.y=0
coord2D normalise(coord2D c)
{
  coord2D o;
  double l = sqrt(c.x * c.x + c.y * c.y);

#define eps 1e-6
  // try to avoid dividing by 0
  if (l > eps)
    {
      o.x = c.x / l;
      o.y = c.y / l;
    }
  else
    {
      o.x = 1;
      o.y = 0;
    }

  return o;
}

coord2D unit_sep(kilobot* bot1, kilobot* bot2)
{
  coord2D us;

  us.x = bot2->x - bot1->x;
  us.y = bot2->y - bot1->y;

  return normalise(us);
}

void collision_detection(int n_bots)
{
  //double r = (double) allbots[0].radius;
  double r = allbots[0]->radius;
  double cr = allbots[0]->cr;

  for (int i=0; i<n_bots; i++) {
    allbots[i]->n_in_range = 0;
  }

  for (int i=0; i<n_bots; i++) {
    for (int j=i+1; j<n_bots; j++) {
      {
	double bd = bot_dist(allbots[i], allbots[j]);
	
	if (bd < (2 * r)) {
	  //	  printf("Whack %d %d\n", i, j);
	  coord2D us = unit_sep(allbots[i], allbots[j]);
	  allbots[i]->x -= us.x;
	  allbots[i]->y -= us.y;
	  allbots[j]->x += us.x;
	  allbots[j]->y += us.y;
	  // we move the bots, this changes the distance.
	  // so bd should be recalculated.
	  // but we only need it below to tell if the bots are
	  // in communications range, and after resolving the collision, they will still be
	  // ... unless they are densely packed and a bot is moved very far, unlikely.
	}
	if (bd < cr) {
	  //if (i == 0) printf("%d and %d in range\n", i, j);
	  allbots[i]->in_range[allbots[i]->n_in_range++] = j;
	  allbots[j]->in_range[allbots[j]->n_in_range++] = i;
	}
      }
    }
  }
}


CommLine commLines[MAXCOMMLINES];
int NcommLines = 0;

void addCommLine(kilobot *from, kilobot *to)
{
  commLines[NcommLines].from = from;
  commLines[NcommLines].to = to;
  commLines[NcommLines].time = 0;
  if (NcommLines < MAXCOMMLINES-1)
    NcommLines++;
}

void removeOldCommLines(int dt, int maxt)
{
  int i;
  for (i = NcommLines-1; i >= 0; i--)
    {
      commLines[i].time += dt;
      if (commLines[i].time > maxt)
	{
	  commLines[i] = commLines[NcommLines-1];
	  NcommLines--;
	}
    }
}



void pass_message(kilobot* tx)
{
  /* Pass message from tx to all bots in range*/

  int i;

  kilo_uid = tx->ID;
  mydata = tx->data;
  message_t * msg = kilo_message_tx();
  distance_measurement_t distm;

  if (msg)
    {
      tx->tx_enabled = 1;
      //printf ("n_in_range=%d\n",tx->n_in_range);
      for (i = 0; i < tx->n_in_range; i++)
	{
	  //switch to next receiving bot
	  kilobot *rx = allbots[tx->in_range[i]];
	  addCommLine(tx, rx);
 
	  kilo_uid = rx->ID;
	  mydata = rx->data;
	  
	  /* set up a distance measurement structure
	   * we know the true distance, so we just store it in the structure
	   * estimate_distance() will just return high_gain .
	   */
	  distm.low_gain = 0;
	  distm.high_gain = bot_dist(tx, rx);
	  
	  kilo_message_rx(msg, &distm);
	}
      
      // switch to the transmitting bot, to call kilo_message_tx_success()
      kilo_uid = tx->ID;
      mydata = tx->data;
      kilo_message_tx_success();
    }
  else
    {
      tx->tx_enabled = 0;
    }
}
 
void process_messaging(int n_bots)
{
  /* Update messaging between bots. */

  for (int i=0; i<n_bots; i++)
    if (kilo_ticks >= allbots[i]->tx_ticks)
      {
	allbots[i]->tx_ticks += tx_period_ticks;
	pass_message(allbots[i]);
      }

  // run removeOldCommLines at most once every kilo_ticks.
  static int last_ticks = 0;
  if (kilo_ticks > last_ticks)
    {
      removeOldCommLines(kilo_ticks-last_ticks, tx_period_ticks);
      last_ticks = kilo_ticks;
    }
}

void update_all_bots(int n_bots, float timestep)
{
  for (int i=0; i<n_bots; i++) {
    update_bot(allbots[i], timestep);
  }

  collision_detection(n_bots);

  process_messaging(n_bots);
}

int GetBotID()
{
  return current_bot;
}
