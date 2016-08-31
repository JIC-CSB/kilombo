/* Core kilobot simulation code.
 */

#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#include <jansson.h>

#include "params.h"
#include "skilobot.h"
#include "kilolib.h"

#include "neighbors.h"

/* Global variables.
 */

// The size (in bytes) of the USERDATA structure.
// Filled in by the user program.
extern int UserdataSize ;

// Variables used to simulate many bots.
kilobot** allbots;
kilobot* current_bot;

// Settings of the simulation.
int tx_period_ticks = 15;  // Message twice a second.

// Callback function pointer for saving the bot's internal state as JSON.
json_t* (*callback_json_state) (void) = NULL;

// Variables used to display communication lines.
CommLine commLines[MAXCOMMLINES];
int NcommLines = 0;


// Function pointers to user defined callback functions.

// These user defined callback functions are called when the relevant function
// keys are pressed during a simulation.
void (*callback_F5) (void) = NULL;
void (*callback_F6) (void) = NULL;

// Callback function prototype and function pointer for simple reporting.
char *botinfo_simple();
char* (*callback_botinfo) (void) = botinfo_simple;

int16_t (*user_obstacles)(double, double, double *, double *) = NULL;
int16_t (*user_light)(double, double) = NULL;
void (*callback_global_setup) (void) = NULL;

/* Dummy functions for messaging. exactly as in kilolib.c */
void message_rx_dummy(message_t *m, distance_measurement_t *d) { }
message_t *message_tx_dummy() { return NULL; }
void message_tx_success_dummy() {}


void set_callback_params(void (*fp)(void))
{
  callback_F5 = fp;
}

void set_callback_reset(void (*fp)(void))
{
  callback_F6 = fp;
}

void set_callback_botinfo(char*(*fp)(void))
{
  callback_botinfo = fp; 
}

void set_callback_json_state(json_t*(*fp)(void))
{
  callback_json_state = fp;
}

void set_callback_global_setup(void(*fp)(void))
{
  callback_global_setup = fp;
}

void set_callback_obstacles(int16_t (*fp)(double, double, double *, double *))
{
	printf("setting user obstacles callback!\n");
  user_obstacles = fp;
}

void set_callback_lighting(int16_t (*fp)(double, double))
{
  user_light = fp;
}
  
// OBSOLETE callback setting functions - will be removed

void register_user_obstacles(int16_t (*fp)(double, double, double *, double *)){
  user_obstacles = fp;
}

void register_user_lighting(int16_t (*fp)(double, double))
{
  user_light = fp;
}

void register_callback(Callback_t type, void (*fp)(void))
{
  // Register a callback. 

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
/* End of obsolete callback setters */



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
  if (simparams->storeHistory)
    {
      bot->x_history = (double *) calloc(bot->n_hist, sizeof(double));
      bot->y_history = (double *) calloc(bot->n_hist, sizeof(double));
    }
  bot->p_hist = 0;
  bot->l_hist = 0;
  
  bot->radius = 17;                  // mm
  bot->leg_angle = 125.5 * M_PI/180; // angle front leg - center - rear leg. 
                                     // 125.5 degrees measured from kilobot PCB design files
  
  bot->right_motor_power = 0;
  bot->left_motor_power = 0;

  bot->speed = simparams->speed;
  bot->turn_rate_l = simparams->turn_rate * M_PI/180; // convert to radians here
  bot->turn_rate_r = simparams->turn_rate * M_PI/180;
  // individual noise could be added here

  
  bot->direction = (2 * M_PI / 4);
  bot->r_led = 0;
  bot->g_led = 0;
  bot->b_led = 0;

  bot->cr = simparams->commsRadius;
            
  bot->in_range = (int*) malloc(sizeof(int) * n_bots);
  bot->n_in_range = 0;

  bot->tx_ticks = rand() % tx_period_ticks;

  bot->user_setup = NULL;
  bot->user_loop  = NULL;
  
  bot->kilo_message_tx = message_tx_dummy;
  bot->kilo_message_tx_success = message_tx_success_dummy;
  bot->kilo_message_rx = message_rx_dummy;

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
    prepare_bot(allbots[i]);
    bot_main();
    finalize_bot(allbots[i]);
  }
}


/* Helper functions for working with the current bot. */
void user_setup_all_bots(int n_bots)
{
  for (int i=0; i<n_bots; i++)
    {
      prepare_bot(allbots[i]);
      current_bot->user_setup();
      finalize_bot(allbots[i]);
    }
}


kilobot *Me()
{
  /* Return the current bot to the calling function.
   *
   * Uses the global variable current_bot to work out which bot is active.
   */

  return current_bot;
}

/* prepare Bot i for running
 * mydata
 * kilo_uid
 */
void prepare_bot(kilobot *bot)
{
  current_bot = bot; 
  kilo_uid                = bot->ID;
  mydata                  = bot->data;
  kilo_message_rx         = bot->kilo_message_rx;
  kilo_message_tx         = bot->kilo_message_tx;
  kilo_message_tx_success = bot->kilo_message_tx_success;
}

/* store the values of kilo_message_* poiners in the per-robot variables
 * to make it possible to have different functions in different bots.
 * Kilolib specifies these to be set by assigning to global variables,
 * to be compatible we have to copy the values.
 */
void finalize_bot(kilobot *bot)
{
  bot->kilo_message_rx         =  kilo_message_rx;
  bot->kilo_message_tx         =  kilo_message_tx;
  bot->kilo_message_tx_success =  kilo_message_tx_success;
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
  /* Dump info for all bots. */

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

  //int velocity = 0.5 * (bot->left_motor_power + bot->right_motor_power);

  bot->y += timestep * bot->speed * cos(bot->direction);
  bot->x += timestep * bot->speed * sin(bot->direction);
}


void turn_bot_right(kilobot *bot, float timestep)
{
  /* Turn the bot to the right by a timestep dependent increment. */

  int r = bot->radius;
  // double x_r = bot->x + r * cos(bot->direction);
  // double y_r = bot->y - r * sin(bot->direction);
  double x_r = bot->x + r * sin(bot->direction + bot->leg_angle);
  double y_r = bot->y + r * cos(bot->direction + bot->leg_angle);
  // bot->direction += timestep * (double) (bot->left_motor_power) / 30;
  bot->direction += timestep * bot->turn_rate_r; 
  // note: turn_rate_r is in radians per sec
  // note: motor power is ignored
  
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
  //  bot->direction -= timestep * (double) (bot->right_motor_power) / 30;
  bot->direction -= timestep * bot->turn_rate_l; 
  // note: turn_rate_r is in radians per sec
  // note: motor power is ignored
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
  if (simparams->storeHistory) {
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

  double p1 = 1, p2 = 1;
  
  int m1 = bot1->left_motor_power || bot1->right_motor_power;
  int m2 = bot2->left_motor_power || bot2->right_motor_power;
  
  if (m1 && !m2)
  	p2 = simparams->pushDisplacement;
  else if (!m1 && m2)
    p1 = simparams->pushDisplacement;

  coord2D suv = separation_unit_vector(bot1, bot2);
  bot1->x -= p1 * suv.x;
  bot1->y -= p1 * suv.y;
  bot2->x += p2 * suv.x;
  bot2->y += p2 * suv.y;
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

  double r = allbots[0]->radius;
  double d_sq = 4*r*r;
  double communication_radius = allbots[0]->cr;
  double communication_radius_sq = communication_radius * communication_radius;
  
  //printf("update_interactions!\n");

  reset_n_in_range_indices(n_bots);

  if (user_obstacles != NULL) {
    double push_x, push_y;

    for (int i=0; i<n_bots; i++) {
      if (user_obstacles(allbots[i]->x, allbots[i]->y, &push_x, &push_y)){
        allbots[i]->x += push_x;
	allbots[i]->y += push_y;
      }
    }
  }

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

// random number between 0 and 1
// NOTE: this can return 1!
double rnd_uniform()
{
  return rand() / (double)RAND_MAX;
}

double rnd_gauss (double mean, double sig)
	{
	double x, y, r2;
	
	do
		{
		// choose x,y in uniform square (-1,-1) to (+1,+1)
		x = -1.0 + 2.0 * rnd_uniform();
		y = -1.0 + 2.0 * rnd_uniform();
		
		// see if it is in the unit circle
		r2 = x * x + y * y;
		}
	while (r2 > 1.0 || r2 == 0.0);
	
	// Box-Muller transform
	return mean + sig * y * sqrt (-2.0 * log (r2) / r2);
	}

/* Simulate a distance measurement
 * with optional gaussian noise and a linear correction.
 * 
 * observations, with bots on whiteboard, not yet simulated 
 * - more noise on long distances, maybe noise proportional to distance-d0
 * - measured distance vs distance starts as linear but flattens out at ~100 mm . 
 */
double noisy_distance(double dist)
{
  double alpha = simparams->distanceCoefficient;
  double d0 = 2 * allbots[0]->radius; 

  // apply linear transformation on the distance.
  // assume that touching bots report the correct distance, and that longer distances scale with alpha.
  dist = alpha*(dist-d0) + d0;

  // add noise
  if (simparams->distance_noise > 0.0)
    dist += rnd_gauss(0, simparams->distance_noise);
  
  return dist > 0 ? dist : 0;
}

int message_success()
{
  return simparams->msg_success_rate >= 1 ? 
    1 : (double)rand() / RAND_MAX <= simparams->msg_success_rate;
}

void pass_message(kilobot* tx)
{
  /* Pass message from tx to all bots in range. */
  distance_measurement_t distm;
  int i;
  prepare_bot(tx);
  //  kilo_uid = tx->ID;
  //  mydata = tx->data;
  message_t * msg = kilo_message_tx();
  finalize_bot(tx);

  if (msg)
    {
      tx->tx_enabled = 1;
      //printf ("n_in_range=%d\n",tx->n_in_range);
      for (i = 0; i < tx->n_in_range; i++) {
	kilobot *rx = allbots[tx->in_range[i]];
#ifndef SKILO_HEADLESS
	if (simparams->GUI)
	  addCommLine(tx, rx);
#endif
	
	if (message_success()) // messages arrive with some probability
	  {
	    /* Set up a distance measurement structure.
	     * We know the true distance, so we just store it in the structure.
	     * estimate_distance() will just return high_gain.
	     */
	    distm.low_gain = 0;
	    distm.high_gain = noisy_distance(bot_dist(tx, rx));
	    
	    prepare_bot(rx);
	    kilo_message_rx(msg, &distm);
	    finalize_bot(rx);
	  }
      }
      
      // Switch to the transmitting bot, to call kilo_message_tx_success().
      prepare_bot(tx);
      kilo_message_tx_success();
      finalize_bot(tx);
    }
  else
    {
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

#ifndef SKILO_HEADLESS
  // Run removeOldCommLines at most once every kilo_ticks.
  static int last_ticks = 0;
  if (simparams->GUI && kilo_ticks > last_ticks) {
    removeOldCommLines(kilo_ticks-last_ticks, tx_period_ticks);
    last_ticks = kilo_ticks;
  }
  #endif
}


/* Functions called by the runsim/headless process_bots function. */

void run_all_bots(int n_bots)
{
  /* Run the user program for each bot. */
  int i;
  for (i=0; i<n_bots; i++) {
    prepare_bot(allbots[i]);
    //printf ("running bot %d.\n", kilo_uid);
    current_bot->user_loop();
    finalize_bot(allbots[i]);
  }
}

void update_all_bots(int n_bots, float timestep)
{
  /* Progress the simulation by a timestep. */

  for (int i=0; i<n_bots; i++) {
    update_bot(allbots[i], timestep);
  }

  if (simparams->useGrid)
    update_interactions_grid(n_bots);
  else
    update_interactions(n_bots);

  process_messaging(n_bots);
}

char botinfo_buffer[500];

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
