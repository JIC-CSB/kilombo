#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#include "skilobot.h"
#include "kilolib.h"

#include "cd_matrix.h"

#include <jansson.h>

kilobot** allbots;
extern int UserdataSize ;  // the size (in bytes) of the USERDATA structure.
                           // filled in by the user program.

int current_bot;

int tx_period_ticks = 15;  // message twice a second

char botinfo_buffer[100];

// default bot_info function, only returns ID
char *botinfo_simple()
{
  sprintf (botinfo_buffer, "%d", kilo_uid);
  return botinfo_buffer;
}

void (*callback_F5) (void) = NULL; // function pointer to user-defined callback function
                                   // this function is called when F5 is pressed
void (*callback_F6) (void) = NULL; // for F6
char* (*callback_botinfo) (void) = botinfo_simple; // function for bot info, returns a string 
json_t* (*callback_json_state) (void) = NULL; //callback for saving the bot's internal state as JSON

void (*callback_global_setup) (void) = NULL;


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
	case CALLBACK_GLOBAL_SETUP:
	  callback_global_setup = fp;
	  break;
    }
}

int storeHistory = 1;

typedef struct {
  double x;
  double y;
} coord2D;

pv_matrix grid_cache;
coord2D gc_offset = {0, 0};
coord2D gc_cell_sz = {50, 50};

size_t bot2gc_x(double x)
	{
	return (x-gc_offset.x) / gc_cell_sz.x;
	}

size_t bot2gc_y(double y)
	{
	return (y-gc_offset.y) / gc_cell_sz.y;
	}

// initialized in update_all_bots before movement
coord2D max_coord, min_coord;


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

  bot->cr = get_int_param("commsRadius", 70);

  bot->in_range = (kilobot**) malloc(sizeof(kilobot*) * n_bots);
  bot->n_in_range = 0;

  bot->tx_ticks = rand() % tx_period_ticks;
  
  bot->data = malloc(UserdataSize);
  
  return bot;
}

void create_bots(int n_bots)
{
  allbots = (kilobot**) malloc(sizeof(kilobot*) * n_bots);

  for (int i=0; i<n_bots; i++) 
    allbots[i] = new_kilobot(i, n_bots);
}

void init_all_bots(int n_bots)
{
  for (int i=0; i<n_bots; i++)
    {
      current_bot = i;      // for Me() to return the right bot
      mydata = Me()->data;
      kilo_uid = i;         // in case the bot's main() uses ID
      bot_main();
    }

  // TODO use extent of initial bot positions
  matrix_init(&grid_cache, 10, 10);
}

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

  double cos_dir = cos(bot->direction);
  double sin_dir = sin(bot->direction);

  double r_cos_dir = r * cos_dir;
  double r_sin_dir = r * sin_dir;
  
  double x_l = bot->x - r_cos_dir;
  double y_l = bot->y + r_sin_dir;
  double x_r = bot->x + r_cos_dir;
  double y_r = bot->y - r_sin_dir;

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
    bot->y += timestep * velocity * cos_dir;
    bot->x += timestep * velocity * sin_dir;
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

  // we need to know the extent of the used area (for the grid cache)
  bot->x > max_coord.x ? (max_coord.x = bot->x) :
	  (bot->x < min_coord.x ? (min_coord.x = bot->x) : 0);
  bot->y > max_coord.y ? (max_coord.y = bot->y) :
	  (bot->y < min_coord.y ? (min_coord.y = bot->y) : 0);
}


double bot_sq_dist(kilobot *bot1, kilobot *bot2)
{
  double x1 = bot1->x;
  double x2 = bot2->x;
  double y1 = bot1->y;
  double y2 = bot2->y;

  return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
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

void prepare_grid_cache(double cr)
	{
	//printf("area: %g, %g - %g, %g\n", min_coord.x, min_coord.y, max_coord.x, max_coord.y);
	// for now we fill the matrix from scratch
	// it might be good to only do that if it had to be extended
	matrix_clear_all(&grid_cache);

	assert(max_coord.x > min_coord.x);
	assert(max_coord.y > min_coord.y);

	// TODO: ceil is dangerous in some (very rare) cases
	size_t x_range = ceil((max_coord.x - min_coord.x + 2*cr)/gc_cell_sz.x);
	size_t y_range = ceil((max_coord.y - min_coord.y + 2*cr)/gc_cell_sz.y);
	
	// this looks like a lot of effort compared to just creating a new matrix
	// but it saves us expensive reallocation of the per-cell arrays
	if (x_range < grid_cache.x_size)
		x_range = grid_cache.x_size;

	if (y_range < grid_cache.y_size)
		y_range = grid_cache.y_size;

	if (x_range > grid_cache.x_size || y_range > grid_cache.y_size)
		matrix_extend(&grid_cache, 0, x_range-grid_cache.x_size, 0, y_range-grid_cache.y_size);

	gc_offset.x = min_coord.x - cr;
	gc_offset.y = min_coord.y - cr;
	}


void store_cache(kilobot * bot)
	{
	assert(bot != NULL);

	p_vec * cell = matrix_get(&grid_cache, bot2gc_x(bot->x), bot2gc_y(bot->y));
	p_vec_push(cell, bot);
	}


void collision_detection(int n_bots)
{

  //double r = (double) allbots[0].radius;
  //double r = allbots[0]->radius;
  double sq_r = allbots[0]->radius * allbots[0]->radius;
  double cr = allbots[0]->cr;
  double sq_cr = cr * cr;

  prepare_grid_cache(cr);

	for (size_t i=0; i<n_bots; i++)
		store_cache(allbots[i]);


  for (int i=0; i<n_bots; i++) {
    allbots[i]->n_in_range = 0;
  }

  for (int i=0; i<n_bots; i++) {

	  kilobot * cur = allbots[i];

	  // range of cells we have to check
	  size_t low_x = bot2gc_x(cur->x - cr);
	  size_t high_x = bot2gc_x(cur->x + cr);
	  size_t low_y = bot2gc_y(cur->y - cr);
	  size_t high_y = bot2gc_y(cur->y + cr);

	  //printf("(%d, %d, %d, %d)", low_x, high_x, low_y, high_y);

	  // FIXME: what about movement? update cache?
	  for (size_t y=low_y; y<=high_y; y++)
		  for (size_t x=low_x; x<=high_x; x++)
			  {
			  p_vec * cell = matrix_get(&grid_cache, x, y);
			  //printf("cs:%d ", cell->size);
			  for (size_t b=0; b<cell->size; b++)
				  {
				  kilobot * other = cell->data[b];
				  //assert(other != NULL);

				  // only process each pair once and don't pair with self
				  if (other <= cur)
					  continue;

  				  double sq_bd = bot_sq_dist(cur, other);
				  if (sq_bd < (4 * sq_r)) {
					  //	  printf("Whack %d %d\n", i, j);
					  coord2D us = unit_sep(cur, other);
					  cur->x -= us.x;
					  cur->y -= us.y;
					  other->x += us.x;
					  other->y += us.y;
					  // we move the bots, this changes the distance.
					  // so bd should be recalculated.
					  // but we only need it below to tell if the bots are
					  // in communications range, and after resolving the collision, 
					  // they will still be
					  // ... unless they are densely packed and a bot is moved very far, unlikely.
					}
				  if (sq_bd < sq_cr) {
					  //if (i == 0) printf("%d and %d in range\n", i, j);
					  cur->in_range[cur->n_in_range++] = other;
					  other->in_range[other->n_in_range++] = cur;
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
	  kilobot *rx = tx->in_range[i];
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
	// make sure we get proper max and min values in update
	max_coord.x = min_coord.x = allbots[0]->x;
	max_coord.y = min_coord.y = allbots[0]->y;

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
