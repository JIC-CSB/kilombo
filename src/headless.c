#include<ctype.h>

#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<math.h>
#include<time.h>

#include"skilobot.h"
#include"params.h"
#include"stateio.h"
#include"kilolib.h"

int w = 1000;
int h = 1000;

/* Dummy functions for messaging. exactly as in kilolib.c */
void message_rx_dummy(message_t *m, distance_measurement_t *d) { }
message_t *message_tx_dummy() { return NULL; }
void message_tx_success_dummy() {}

void distribute_line(int n_bots)
{
  for (int i=0; i < n_bots; i++) {
    allbots[i]->x += 80 * i - 40 * (n_bots - 1);

  }
}

void distribute_rand(int n_bots, int w, int h)
{

  for (int i=0; i < n_bots; i++) {
    allbots[i]->x = rand()%w - w/2;
    allbots[i]->y = rand()%h - h/2;
    allbots[i]->direction = 2 * M_PI * (float) rand() / (float) RAND_MAX;
  }
}

void initialise_simulator(const char *param_filename)
{
  /* Parse parameter file and perform initialisation */

  simparams = parse_param_file(param_filename);

  int rand_seed = get_int_param("randSeed", 0);

  if (!rand_seed) {
    srand(time(0));
  } else {
    srand(rand_seed);
  }

}

void initialise_bots(int n_bots)
{
  init_all_bots(n_bots);

  //distribute_line(n_bots);
  //distribute_rand(n_bots, w, h);
  //int w = get_int_param("distributeWidth", 500);
  //int h = get_int_param("distributeHeight", 500);
  float f = get_float_param("distributePercent", 0.2);
  distribute_rand(n_bots, f * w, f * h);
  
}

void process_bots(int n_bots, float timestep)
{
    run_all_bots(n_bots);
    update_all_bots(n_bots, timestep);
}

float cbot_dist(kilobot *bot1, kilobot *bot2)
{
  float x1 = bot1->x;
  float x2 = bot2->x;
  float y1 = bot1->y;
  float y2 = bot2->y;

  return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

float calc_dists(int n_bots)
{
  float sum_dist = 0;
  for (int i=0; i<n_bots; i++) {
    for (int j=i; j<n_bots; j++) {
      if (i != j) {
	sum_dist += cbot_dist(allbots[i], allbots[j]);
      }
    }
  }

  return sum_dist;
}

/* main() is defined as bot_main in kilolib.h
   to rename the bot's main function. This is the real main(),
   so we need to get rid of the definition*/
#undef main

int main(int argc, char *argv[])
{
  int n_bots = 100;
  float time = 0;
  float d_init;

  char param_filename[255] = "simulatorParams.json";
  char *bot_state_file = (char *) NULL;

  int c;
  
  while ((c = getopt(argc, argv, "n:p:b:")) != -1) {
    switch (c) {
    case 'n':
      n_bots = atoi(optarg);
      break;
    case 'p':
      strncpy(param_filename, optarg, 255);
      break;
    case 'b':
      bot_state_file = (char *) malloc(255 * sizeof(char));
      strncpy(bot_state_file, optarg, 255);
      break;
    default:
      abort();
    }
  }

  initialise_simulator(param_filename);

  if (!simparams) {
    fprintf(stderr, "Couldn't load parameter file\n");
    return 1;
  }

  kilo_message_tx = message_tx_dummy;
  kilo_message_tx_success = message_tx_success_dummy;
  kilo_message_rx = message_rx_dummy;
  
  if (bot_state_file) {
    allbots = bot_loader(bot_state_file, &n_bots);
  } else {
    n_bots = get_int_param("nBots", n_bots);
    initialise_bots(n_bots);    
  }

  dump_all_bots(n_bots);

  float maxTime = get_float_param("simulationTime", 100.0);
  float timeStep = get_float_param("timeStep", 0.02);

  printf("Running with timestep %f for total time %f\n", timeStep, maxTime);

  int n_timesteps = (int) (1 + (maxTime / timeStep));

  int n_step = 0;
  while(time < maxTime) {
     //printf("Time = %f (pause %d)\n", t, pause);

    process_bots(n_bots, timeStep);

    time += timeStep;
    n_step++;

  }

  save_bot_state_to_file(allbots, n_bots, "endstate.json");

  return 0;
}
