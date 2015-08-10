#include<ctype.h>

#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<math.h>
#include<time.h>
#include<jansson.h>

#include"kilolib.h"
#undef main 
/* main() is defined as bot_main in kilolib.h
   to rename the bot's main function. This file has the real main(),
   so we need to get rid of the definition. Also, SDL redefines main as 
   SDL_main on OSX, so SDL_main.h needs to be included after this.
*/


#include"skilobot.h"
#include"params.h"
#include"stateio.h"



int n_bots = 100;
int state = RUNNING;
int fullSpeed = 0;     // if nonzero, run without delay between frames

extern int storeHistory;  //whether to save the history of all bot positions

void distribute_bots(int n_bots);
extern void (*callback_global_setup) (void);

/* Dummy functions for messaging. exactly as in kilolib.c */
void message_rx_dummy(message_t *m, distance_measurement_t *d) { }
message_t *message_tx_dummy() { return NULL; }
void message_tx_success_dummy() {}


void die(char *s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

void initialise_simulator(const char *param_filename)
{
  /* Parse parameter file and perform initialisation */

  // Read simulator parameters from JSON,
  // fill in the global simparams structure
  parse_param_file(param_filename);

  if (!simparams->randSeed) {
    srand(time(0));
  } else {
    srand(simparams->randSeed);
  }
}


int main(int argc, char *argv[])
{
  n_bots = 100;
  double time = 0;
  char *bot_state_file = (char *) NULL;
  int c;  
  char param_filename[1000];
  sprintf (param_filename, "%s%s", argv[0], ".json");
  //default to <program name>.json
  
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

  allbots = NULL;
  if (bot_state_file) {
    allbots = bot_loader(bot_state_file, &n_bots);
    if (allbots == NULL)
	die("Could not parse the given bot file");
  }
  
  if (allbots == NULL) //no bot file given, or it could not be parsed
    {
      n_bots = get_int_param("nBots", n_bots);

      if (n_bots <= 0)
	die("nBots must be > 0.");
      
      create_bots(n_bots);
      distribute_bots(n_bots);
    }

  // call main() in every bot
  init_all_bots(n_bots);
  
  // dump_all_bots(n_bots);

  // maxTime <= 0 for unlimited simulation

  // call user-supplied global setup after reading parameters but before
  // doing any real work
  if (callback_global_setup != NULL)
	  callback_global_setup();

  // call the per-bot setup here so that global setup can provide
  // e.g. simulation-specific parameter values to it
  user_setup_all_bots(n_bots);


  json_t *j_state = json_array();
    
  printf("Running %d bots with timestep %f for total time %f\n", 
	 n_bots, simparams->timeStep, simparams->maxTime);

  int n_step = 0;

  while(time < simparams->maxTime || simparams->maxTime <= 0)
    {
    //   printf("-- %d  kilo_ticks:%d--\n", n_step, kilo_ticks);

      // Do one time step
      process_bots(n_bots, simparams->timeStep);
      n_step++;
      time += simparams->timeStep;
      kilo_ticks = time * TICKS_PER_SEC;
      
      // save simulation state as JSON
      if (simparams->stateFileName && n_step % simparams->stateFileSteps == 0)
	{
	  // printf("Saving state to JSON at %6d steps\n", n_step);
	  json_t *t = json_rep_all_bots(allbots, n_bots, kilo_ticks);
	  json_array_append(j_state, t);
	}
    } 

  save_bot_state_to_file(allbots, n_bots, "endstate.json");

  if (simparams->stateFileName)
    {
      json_dump_file(j_state, simparams->stateFileName, JSON_INDENT(2) | JSON_SORT_KEYS);
    }  
  return 0;
}
