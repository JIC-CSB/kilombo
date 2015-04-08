#include<ctype.h>

#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<math.h>
#include<time.h>

#include"kilolib.h"
#undef main 
/* main() is defined as bot_main in kilolib.h
   to rename the bot's main function. This file has the real main(),
   so we need to get rid of the definition. Also, SDL redefines main as SDL_main on OSX,
   so SDL_main.h needs to be included after this.
*/

#include <SDL/SDL.h>
#include <SDL/SDL_main.h>
#include "gfx/SDL_framerate.h"

#include"skilobot.h"
#include"display.h"
#include"params.h"
#include"stateio.h"



int n_bots = 100;
int state = RUNNING;
int fullSpeed = 0;   //if nonzero, run without delay between frames

extern int storeHistory;  //whether to save the history of all bot positions



/* Dummy functions for messaging. exactly as in kilolib.c */
void message_rx_dummy(message_t *m, distance_measurement_t *d) { }
message_t *message_tx_dummy() { return NULL; }
void message_tx_success_dummy() {}


void die(char *s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

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
    allbots[i]->direction = 2 * 3.141 * (float) rand() / (float) RAND_MAX;
  }
}
void distribute_pile2(int n_bots)
{
  int pos_x=0;
  int pos_y=0;
  int radius = allbots[0]->radius;
  
  int num_rows=ceil(sqrt(n_bots));
  float num_start=ceil(num_rows/2);
  int cont=0;

  for (int i=-num_start; i < num_start; i++) {
    for (int j=-num_start; j < num_start; j++) {
      
      
      pos_x = i*2*(radius+3);
      pos_y = j*2*(radius+3);
      
      if (cont<n_bots)
	{
	  allbots[cont]->x = pos_x;
	  allbots[cont]->y = pos_y;
	  allbots[cont]->direction = ((float)rand()/(float)(RAND_MAX)) * (2*3.1415927);
	  //allbots[cont]->direction = 0;
	}
      cont++;
      
    }
  }
}

// move the bots apart slightly, by lettign them repel each other
void spread_out(double k)
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

void special_init(int n_bots)
{
  /* Run specific initialisation */
  // Maybe this sould go int the user program file ? - FJ

  /*
  for (int i=0; i < n_bots; i++) {
    allbots[i]->type = 1 + (i % 3);
    } */
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
  distribute_rand(n_bots, f * display_w, f * display_h);
  //distribute_pile2(n_bots);

  special_init(n_bots);

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



int main(int argc, char *argv[])
{
  n_bots = 100;
  float time = 0;

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

  screen = makeWindow(display_w, display_h);

  kilo_message_tx = message_tx_dummy;
  kilo_message_tx_success = message_tx_success_dummy;
  kilo_message_rx = message_rx_dummy;

  if (bot_state_file) {
    allbots = bot_loader(bot_state_file, &n_bots);
    int i;
    //call bot_main() for each bot
    for (i = 0; i < n_bots; i++)  
      {
	current_bot = i;      // for Me() to return the right bot
	mydata = Me()->data;
	kilo_uid = i;         // in case the bot's main() uses ID
	bot_main();
      }
  } else {
    n_bots = get_int_param("nBots", n_bots);
    if (n_bots <= 0)
      die("nBots must be > 0.");
	
    initialise_bots(n_bots);    
  }

  //  dump_all_bots(n_bots);

  // maxTime <= 0 for unlimited simulation
  float maxTime   = get_float_param("simulationTime", 0);
  float timeStep  = get_float_param("timeStep", 0.02);
  const char *imageName = get_string_param("imageName", NULL);
  storeHistory = get_int_param("storeHistory", 1);
  int saveVideoN = get_int_param("saveVideoN", 1); // save video screenshot every Nth frame
  
  char buf[2000];

  FPSmanager manager;
  SDL_initFramerate(&manager);
  double FPS = 1.0 / timeStep;
  SDL_setFramerate(&manager, FPS);
  


  printf("Running %d bots with timestep %f for total time %f\n", 
	 n_bots, timeStep, maxTime);

  //printf("Time,Dist\n");

  //  int n_timesteps = (int) (1 + (maxTime / timeStep));

  int n_step = 0;
  while(!quit && (time < maxTime || maxTime <= 0)) {
     //printf("Time = %f (pause %d)\n", t, pause);

    input(); // process input here for fastest response.

    SDL_FillRect(screen, NULL, 0x00000000);
    if (get_int_param("showComms", 1)) 
      draw_commLines(screen);
    for (int i=0; i <n_bots; i++) {
      draw_bot(screen, display_w, display_h, allbots[i]);
    }
    
    
    //    printf("-- %d  kilo_ticks:%d--\n", n_step, kilo_ticks);
    
    if (state == RUNNING)
      {
	process_bots(n_bots, timeStep);

	time += timeStep;
	n_step++;
      }    

   
    
    if (imageName && saveVideo)
      if (n_step % saveVideoN == 0)
	{
	  static int frame = 0;
	  snprintf (buf, 2000, imageName, frame);
	  frame++;
	  SDL_SaveBMP(screen, buf);
	}

    kilo_ticks = time * TICKS_PER_SEC;
    draw_status(screen, display_w, display_h);
    // draw status here, after video capture
    
    SDL_Flip(screen);
    if (!fullSpeed)
      SDL_framerateDelay(&manager);
  }

  save_bot_state_to_file(allbots, n_bots, "endstate.json");

  return 0;
}
