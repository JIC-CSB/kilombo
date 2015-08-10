/* Core kilobot simulation code.
 */

#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#include <jansson.h>

#include "params.h"
#include "skilobot.h"
#include "kilolib.h"


/* Global variables.
 */

// The size (in bytes) of the USERDATA structure.
// Filled in by the user program.
extern int UserdataSize ;

// Variables used to simulate many bots.
kilobot** allbots;
int current_bot;

// Settings of the simulation.
int tx_period_ticks = 15;  // Message twice a second.
int storeHistory = 1;

// Function pointers to user defined callback functions.
// The user defined callback functions are called when the relevant function
// keys are pressed during a simulation.
void (*callback_F5) (void) = NULL;
void (*callback_F6) (void) = NULL;

// Callback function prototype and function pointer for simple reporting.
char *botinfo_simple();
char* (*callback_botinfo) (void) = botinfo_simple;

// Callback function pointer for saving the bot's internal state as JSON.
json_t* (*callback_json_state) (void) = NULL;

// Variables used to display communication lines.
CommLine commLines[MAXCOMMLINES];
int NcommLines = 0;

void (*callback_global_setup) (void) = NULL;

/* Functions.
 */


void register_callback(Callback_t type, void (*fp)(void))
{
  /* Register a callback. */

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
	case CALLBACK_GLOBAL_SETUP:
	  callback_global_setup = fp;
	  break;
    }
}


/* Functions for initialising the bots to be used in the simulation. */

kilobot *new_kilobot(int ID, int n_bots)
{
  /* Allocates the memory for a kilobot struct and populates it with default
   * values.*/

  kilobot* bot = (kilobot*) calloc(1, sizeof(kilobot));
  // calloc sets the memory area to 0 - guarantees initialization of user data.

  bot->ID = ID;
  bot->x = 0;
  bot->y = 0;

  bot->n_hist = simparams->histLength;
  bot->x_history = (double *) calloc(bot->n_hist, sizeof(double));
  bot->y_history = (double *) calloc(bot->n_hist, sizeof(double));
  bot->p_hist = 0;
  bot->l_hist = 0;
  
  bot->radius = 17;                  // mm
  bot->leg_angle = 125.5 * M_PI/180; // angle front leg - center - rear leg. 
                                     // 125.5 degrees measured from kilobot PCB design files
  
  bot->right_motor_power = 0;
  bot->left_motor_power = 0;
  bot->direction = (2 * M_PI / 4);
  bot->r_led = 0;
  bot->g_led = 0;
  bot->b_led = 0;

  bot->cr = simparams->commsRadius;
            
  bot->in_range = (int*) malloc(sizeof(int) * n_bots);
  bot->n_in_range = 0;

  bot->tx_ticks = rand() % tx_period_ticks;
  
  bot->data = malloc(UserdataSize);
  
  return bot;
}

void create_bots(int n_bots)
{
  /*
   * Allocate the memory required to store n pointers to bots and
   * creates n new kilobots.
   */

  allbots = (kilobot**) malloc(sizeof(kilobot*) * n_bots);

  for (int i=0; i<n_bots; i++) {
    allbots[i] = new_kilobot(i, n_bots);
  }
}

void init_all_bots(int n_bots)
{
  /* Call the setup function of the user's bot. */

  for (int i=0; i<n_bots; i++) {
    current_bot = i;      // For Me() to return the right bot.
    mydata = Me()->data;
    kilo_uid = i;         // In case the bot's main() uses ID.
    bot_main();
  }
}


/* Helper functions for working with the current bot. */
void user_setup_all_bots(int n_bots)
{
  for (int i=0; i<n_bots; i++)
    {
      current_bot = i;      // for Me() to return the right bot
      mydata = Me()->data;
      kilo_uid = i;         // in case the bot's main() uses ID
	  user_setup();
    }
}


kilobot *Me()
{
  /* Return the current bot to the calling function.
   *
   * Uses the global variable current_bot to work out which bot is active.
   */

  return allbots[current_bot];
}


/* Functions for logging output to stdout. */

void dump_bot_info(kilobot *self)
{
  /* Dump bot info to stdout. */

  printf("B %d: At (%f, %f), speed (%d, %d)\n", 
    self->ID, self->x, self->y, self->right_motor_power, self->left_motor_power);
}

void dump_all_bots(int n_bots)
{
  /* Dump info for all bots. Should only be called in a critical region. */

  for (int i=0; i<n_bots; i++) {
    dump_bot_info(allbots[i]);
  }
}


/* Functions for managing the history of the bots. */

void manage_bot_history_memory(kilobot *bot)
{
  /* If our history is longer than the memory allocation, reallocate. */

  if (1 + bot->p_hist > bot->n_hist) {
    bot->n_hist += 100;
    bot->x_history = (double *) realloc(bot->x_history, sizeof(double) * bot->n_hist);
    bot->y_history = (double *) realloc(bot->y_history, sizeof(double) * bot->n_hist);
  }
}

void update_bot_history(kilobot *bot)
{
  /* Update the bot's history of where it has been. */

  bot->x_history[bot->p_hist] = bot->x;
  bot->y_history[bot->p_hist] = bot->y;
  bot->p_hist++;

  manage_bot_history_memory(bot);
}

void update_bot_history_ring(kilobot *bot)
{
  /* Update the bot's history of where it has been, using a ring buffer,
     of size simparams->histLength */

  bot->p_hist %= simparams->histLength;
  bot->x_history[bot->p_hist] = bot->x;
  bot->y_history[bot->p_hist] = bot->y;
  bot->p_hist++;

  // count valid history entries in the buffer 
  if (bot->l_hist < simparams->histLength)
    bot->l_hist++;
}


/* Functions for moving the bots. */

void move_bot_forward(kilobot *bot, float timestep)
{
  /* Move the bot forwards by a timestep dependent increment. */

  int velocity = 0.5 * (bot->left_motor_power + bot->right_motor_power);
  bot->y += timestep * velocity * cos(bot->direction);
  bot->x += timestep * velocity * sin(bot->direction);
}


void turn_bot_right(kilobot *bot, float timestep)
{
  /* Turn the bot to the right by a timestep dependent increment. */

  int r = bot->radius;
  // double x_r = bot->x + r * cos(bot->direction);
  // double y_r = bot->y - r * sin(bot->direction);
  double x_r = bot->x + r * sin(bot->direction + bot->leg_angle);
  double y_r = bot->y + r * cos(bot->direction + bot->leg_angle);
  bot->direction += timestep * (double) (bot->left_motor_power) / 30;
  bot->x = x_r - r * sin(bot->direction + bot->leg_angle);
  bot->y = y_r - r * cos(bot->direction + bot->leg_angle);
}

void turn_bot_left(kilobot *bot, float timestep)
{
  /* Turn the bot to the left by a timestep dependent increment. */

  int r = bot->radius;
  // double x_l = bot->x - r * cos(bot->direction);
  // double y_l = bot->y + r * sin(bot->direction);
  double x_l = bot->x + r * sin(bot->direction - bot->leg_angle);
  double y_l = bot->y + r * cos(bot->direction - bot->leg_angle);
  bot->direction -= timestep * (double) (bot->right_motor_power) / 30;
  bot->x = x_l - r * sin(bot->direction - bot->leg_angle);
  bot->y = y_l - r * cos(bot->direction - bot->leg_angle);
}

void update_bot_location(kilobot *bot, float timestep)
{
  /* Update the bot's location by a timestep dependent increment. */
  if (bot->right_motor_power && bot->left_motor_power) { // forward movement
    move_bot_forward(bot, timestep);
  }
  else {
    if (bot->left_motor_power) {
      turn_bot_right(bot, timestep);
    }
    if (bot->right_motor_power) {
      turn_bot_left(bot, timestep);
    }
  }
}


/* Functions for updating both the history and the location of the bots. */

void update_bot(kilobot *bot, float timestep)
{
  /* Update a bot by a timestep dependent increment.
   *
   * Save the history (if required) before updating the location.
   */
  if (storeHistory) {
    update_bot_history_ring(bot);
  }
  update_bot_location(bot, timestep);
}


/* Geometry helper functions. */

double bot_dist(kilobot *bot1, kilobot *bot2)
{
  /* Return the bot2bot distance. */

  double x1 = bot1->x;
  double x2 = bot2->x;
  double y1 = bot1->y;
  double y2 = bot2->y;

  return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

double bot_sq_dist(kilobot *bot1, kilobot *bot2)
{
  double x1 = bot1->x;
  double x2 = bot2->x;
  double y1 = bot1->y;
  double y2 = bot2->y;

  return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}


coord2D normalise(coord2D c)
{
  /* Return a unit vector parallel to c (treating c as a vector).
   *
   * If c is 0 or very short, special case: choose o.x=1, o.y=0.
   */

  coord2D o;
  double l = sqrt(c.x * c.x + c.y * c.y);

#define eps 1e-6
  // Try to avoid dividing by 0.
  if (l > eps) {
    o.x = c.x / l;
    o.y = c.y / l;
  }
  else {
    o.x = 1;
    o.y = 0;
  }

  return o;
}

coord2D separation_unit_vector(kilobot* bot1, kilobot* bot2)
{
  /* Return the separation unit vector between the two bots. */

  coord2D separation_vector;

  separation_vector.x = bot2->x - bot1->x;
  separation_vector.y = bot2->y - bot1->y;

  return normalise(separation_vector);
}

void separate_clashing_bots(kilobot* bot1, kilobot* bot2)
{
  /* Move bot1 and bot2 apart.
   *
   * Each bot moves one unit away from the other.
   */

  coord2D suv = separation_unit_vector(bot1, bot2);
  bot1->x -= suv.x;
  bot1->y -= suv.y;
  bot2->x += suv.x;
  bot2->y += suv.y;
}


/* Functions for determining which bots can communicate with each other. */

void reset_n_in_range_indices(int n_bots)
{
  /* Set all n_in_range variables to 0.
   *
   * I.e. after this function has been called the bots will have no bots within
   * communication radius.
   */

  for (int i=0; i<n_bots; i++) {
    allbots[i]->n_in_range = 0;
  }
}

void update_n_in_range_indices(kilobot* bot1, kilobot* bot2)
{
  /* Set bot1 and bot2 to be within commuication radius of each other
   * and increment the n_in_range counters. */

  bot1->in_range[bot1->n_in_range++] = bot2->ID;
  bot2->in_range[bot2->n_in_range++] = bot1->ID;
}


/* Functions for dealing with interactions between bots. */

void update_interactions(int n_bots)
{
  /* Update the bots' interactions with each other.
   *
   * - Move clashing bots apart.
   * - Update which bots can communicate with each other.
   */

  //double r = (double) allbots[0].radius;
  double r = allbots[0]->radius;
  double d_sq = 4*r*r;
  double communication_radius = allbots[0]->cr;
  double communication_radius_sq = communication_radius * communication_radius;
  
  reset_n_in_range_indices(n_bots);

  for (int i=0; i<n_bots; i++) {
    for (int j=i+1; j<n_bots; j++) {
      double bot2bot_sq_distance = bot_sq_dist(allbots[i], allbots[j]);

      if (bot2bot_sq_distance < d_sq) {
        //printf("Whack %d %d\n", i, j);
        separate_clashing_bots(allbots[i], allbots[j]);
        // We move the bots, this changes the distance.
        // So bot2bot_distance should be recalculated.
        // But we only need it below to tell if the bots are
        // in communications range, and after resolving the collision,
        // they will still be...
        // Unless they are densely packed and a bot is moved
        // very far, which is unlikely.
      }
      if (bot2bot_sq_distance < communication_radius_sq) {
        //if (i == 0) printf("%d and %d in range\n", i, j);
        update_n_in_range_indices(allbots[i], allbots[j]);
      }
    }
  }
}

void addCommLine(kilobot *from, kilobot *to)
{
  /* Add a communication line between two bots. */

  commLines[NcommLines].from = from;
  commLines[NcommLines].to = to;
  commLines[NcommLines].time = 0;
  if (NcommLines < MAXCOMMLINES-1) {
    NcommLines++;
  }
}

void removeOldCommLines(int dt, int maxt)
{
  /* Remove old communication lines. */

  int i;
  for (i = NcommLines-1; i >= 0; i--) {
    commLines[i].time += dt;
    if (commLines[i].time > maxt) {
      commLines[i] = commLines[NcommLines-1];
      NcommLines--;
    }
  }
}

void pass_message(kilobot* tx)
{
  /* Pass message from tx to all bots in range. */

  int i;

  kilo_uid = tx->ID;
  mydata = tx->data;
  message_t * msg = kilo_message_tx();
  distance_measurement_t distm;

  if (msg) {
    tx->tx_enabled = 1;
    //printf ("n_in_range=%d\n",tx->n_in_range);
    for (i = 0; i < tx->n_in_range; i++) {
      // Switch to next receiving bot.
      kilobot *rx = allbots[tx->in_range[i]];
      addCommLine(tx, rx);
 
      kilo_uid = rx->ID;
      mydata = rx->data;

     /* Set up a distance measurement structure.
      * We know the true distance, so we just store it in the structure.
      * The estimate_distance() will just return high_gain.
      */
      distm.low_gain = 0;
      distm.high_gain = bot_dist(tx, rx);

      kilo_message_rx(msg, &distm);
    }

    // Switch to the transmitting bot, to call kilo_message_tx_success().
    kilo_uid = tx->ID;
    mydata = tx->data;
    kilo_message_tx_success();
  }
  else {
    tx->tx_enabled = 0;
  }
}
 
void process_messaging(int n_bots)
{
  /* Update messaging between bots. */

  for (int i=0; i<n_bots; i++) {
    if (kilo_ticks >= allbots[i]->tx_ticks) {
      allbots[i]->tx_ticks += tx_period_ticks;
      pass_message(allbots[i]);
    }
  }

  // Run removeOldCommLines at most once every kilo_ticks.
  static int last_ticks = 0;
  if (kilo_ticks > last_ticks) {
    removeOldCommLines(kilo_ticks-last_ticks, tx_period_ticks);
    last_ticks = kilo_ticks;
  }
}


/* Functions called by the runsim/headless process_bots function. */

void run_all_bots(int n_bots)
{
  /* Run the user program for each bot. */

  for (current_bot=0; current_bot<n_bots; current_bot++) {
    //me = & (allbots[current_bot]->data);

    //  alternative way for each bot to access its own data - less copying - FJ
    mydata = allbots[current_bot]->data;
    kilo_uid = allbots[current_bot]->ID;
    //printf ("running bot %d ID: %d\n", current_bot, kilo_uid);
    user_loop();
  }
}

void update_all_bots(int n_bots, float timestep)
{
  /* Progress the simulation by a timestep. */

  for (int i=0; i<n_bots; i++) {
    update_bot(allbots[i], timestep);
  }

  update_interactions(n_bots);

  process_messaging(n_bots);
}

char botinfo_buffer[100];

//default callback_botinfo function
char *botinfo_simple()
{
  sprintf (botinfo_buffer, "%d", kilo_uid);
  return botinfo_buffer;
}

void spread_out(int n_bots, double k)
{
  int i, j;
  for (j = 0; j < n_bots; j++)
    for (i = 0; i < j; i++)
      {
	double dx = allbots[i]->x - allbots[j]->x;
	double dy = allbots[i]->y - allbots[j]->y;
	double r = hypot(dx, dy);

	// arbitrary cap to avoid large forces
	if (r < 5)
	  r = 5;
	
	// a force
	double fx = 1/(r*r) * dx/r ;
	double fy = 1/(r*r) * dy/r;

	allbots[i]->x += fx * k;
	allbots[i]->y += fy * k;
	allbots[j]->x -= fx * k;
	allbots[j]->y -= fy * k;	
      }
}

void process_bots(int n_bots, float timestep)
{
    run_all_bots(n_bots);
    update_all_bots(n_bots, timestep);
}
