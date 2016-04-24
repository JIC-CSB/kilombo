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

   #ifndef SKILO_HEADLESS
#include<SDL/SDL.h>
#include<SDL/SDL_main.h>
#include"gfx/SDL_framerate.h"
#include"display.h"
   #endif

#include"skilobot.h"
#include"params.h"
#include"stateio.h"

// timing macros.
// http://stackoverflow.com/questions/173409/how-can-i-find-the-execution-time-of-a-section-of-my-program-in-c
clock_t startm, stopm;
#define START if ( (startm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define STOP if ( (stopm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define PRINTTIME printf( "%6.3f s.", ((double)stopm-startm)/CLOCKS_PER_SEC);

int n_bots = 100;
int state = RUNNING;
int fullSpeed = 0;     // if nonzero, run without delay between frames


void distribute_bots(int n_bots);
extern void (*callback_global_setup) (void);


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
#ifndef SKILO_HEADLESS
  set_display_center(simparams->displayX, simparams->displayY);
#endif
}

#ifndef SKILO_HEADLESS
void draw()
{
  SDL_FillRect(screen, NULL, colorscheme->background);
  
  for (int i=0; i <n_bots; i++) 
    draw_bot_history_ring(screen, simparams->display_w, simparams->display_h, allbots[i]);
  
  if (simparams->showComms) 
    draw_commLines(screen);
  
  for (int i=0; i <n_bots; i++) 
    draw_bot(screen, simparams->display_w, simparams->display_h, allbots[i]);
}
#endif

int main(int argc, char *argv[])
{
  n_bots = 100;
  double time = 0;
  char *bot_state_file = (char *) NULL;
  int c;  
  char param_filename[1000] = "kilombo.json"; // Default parameter file name 
  
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

#ifndef SKILO_HEADLESS
  double frameTimeAvg = 0;

  init_SDL();
  if (simparams->GUI)
    screen = makeWindow();
  else
    screen = makeSurface();
    // when running without a GUI, make an off-screen SDL surface for
    // saving video or screenshots

  const char *s = get_string_param("colorscheme", NULL);
  if (s != NULL)
    {
      //      printf("Colorscheme chosen: %s\n", s);
      if (strcasecmp(s, "bright") == 0)
	{
	  //  printf("bright colors selected.\n");
	  colorscheme = &brightColors;
	}
    }
#endif
  
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


#ifndef SKILO_HEADLESS
  FPSmanager manager;
  SDL_initFramerate(&manager);
  double FPS = 1.0 / simparams->timeStep;
  SDL_setFramerate(&manager, FPS);
  int steps_since_draw = 0;

  Uint32 lastTicks;
  lastTicks = SDL_GetTicks();
  char buf[2000];  
#endif
  
  json_t *j_state = json_array();

  printf ("Size of kilobot structure : %zd\n", sizeof(kilobot));
  extern int UserdataSize;
  printf ("Size of USERDATA structure: %d\n", UserdataSize);
  
  printf("Running %d bots with timestep %f for total time %f\n", 
	 n_bots, simparams->timeStep, simparams->maxTime);

  int n_step = 0;

  START
  
  while(time < simparams->maxTime || simparams->maxTime <= 0) {
    //printf("-- step:%d  kilo_ticks:%d  time:%6.1f--\n", n_step, kilo_ticks, time);

    if (state == RUNNING && simparams->stepsPerFrame > 0)
      {
	// Do one time step
	process_bots(n_bots, simparams->timeStep);
	time += simparams->timeStep;
	kilo_ticks = time * TICKS_PER_SEC;
       
	// save simulation state as JSON
	if (simparams->stateFileSteps != 0)
	  if (simparams->stateFileName && n_step % simparams->stateFileSteps == 0)
	    {
	      // printf("Saving state to JSON at %6d steps\n", n_step);
	      json_t *t = json_rep_all_bots(allbots, n_bots, kilo_ticks);
	      json_array_append_new(j_state, t);
	    }

#ifndef SKILO_HEADLESS	
	// save screenshots for video
	if (simparams->imageName && simparams->saveVideo)
	  if (n_step % simparams->saveVideoN == 0)
	    {
	      draw(); 
	      static int frame = 0;
	      snprintf (buf, 2000, simparams->imageName, frame);
	      // printf("Saving video screenshot to %s at %6d steps\n", buf, n_step);
	      frame++;
	      if (SDL_SaveBMP(screen, buf))
		{
		  fprintf(stderr, "Error saving video frame to file %s\n", buf);
		  exit(1);
		}
	    }
#endif
	if (n_step % 1000 == 0)
	  {
	    STOP
	      printf("%6.0f s - %6d steps - %6d kilo_ticks   ", time, n_step, kilo_ticks);
	    PRINTTIME
	      printf ("\n");

	    START
	  }
      } // if RUNNING

#ifndef SKILO_HEADLESS	
    steps_since_draw++;
    // Draw on screen
    if (simparams->GUI)
      if (steps_since_draw >= simparams->stepsPerFrame  || state != RUNNING || simparams->stepsPerFrame == 0)
	{     // avoiding n_step % stepsPerFrame here because of weird behaviour when changing speed
	  steps_since_draw = 0;
	  input();
	  if (quit)
	    break;
	  draw(); // The same frame may already be drawn for video, drawing again for simplicity
	  // Draw status message on screen but not in video
	  draw_status(screen, simparams->display_w, simparams->display_h, time, 1000.0/frameTimeAvg);
	  
	  SDL_Flip(screen);
	  if (!fullSpeed)
	    SDL_framerateDelay(&manager);
	  
	  // rolling average of the time per frame, just for display
	  Uint32 t = SDL_GetTicks();
	  double w = .02;
	  frameTimeAvg = (1-w) * frameTimeAvg + w*(t-lastTicks); 
	  lastTicks = t;    
	}
#endif

  // increment step here so that state is printed at t=0
  n_step++;
  } // while running

  printf ("Simulation finished\n");
  
  save_bot_state_to_file(allbots, n_bots, "endstate.json");

  if (simparams->stateFileName && simparams->stateFileSteps != 0)
    {
      json_dump_file(j_state, simparams->stateFileName, JSON_INDENT(2) | JSON_SORT_KEYS);
	  json_decref(j_state);
    }

#ifndef SKILO_HEADLESS	
  if (simparams->finalImage)
    {
      draw();
      if (SDL_SaveBMP(screen, simparams->finalImage))
	{
	  fprintf(stderr, "Error saving video frame to file %s\n", simparams->finalImage);
	  exit(1);
	}
    }
#endif
  
  return 0;
}
