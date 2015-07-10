#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kilolib.h"
#undef main   // to avoid warning when SDL redefines main

#include <SDL/SDL.h>
#include "display.h"
#include "params.h"
#include "rom8x8.h"
#include "gfx/SDL_gfxPrimitives.h"
#include "SDL/SDL_thread.h"
#include "SDL/SDL_timer.h"
#include "skilobot.h"


//for mkdir
#define _XOPEN_SOURCE 700
#include <sys/stat.h>
#include <sys/types.h>
//windows: #include <direct.h> and use _mkdir, or use cygwin



extern void (*callback_F5) (void) ; // function pointer to user-defined callback function for F5 press
extern void (*callback_F6) (void) ; // function pointer to user-defined callback function for F6 press
                                    // run for *ALL* bots in sequence, used for reset
extern char* (*callback_botinfo) (void) ; // function pointer to user-defined callback function for bot info

SDL_Surface *screen;

ColorScheme darkColors = {
  .background     = 0x00000000,
  .text           = 0xffffffff,
  .bot_border     = 0xffffffff,
  .bot_left_leg   = 0xffaaaaff,
  .bot_right_leg  = 0xaaffaaff,
  .bot_line_front = 0x0000ffff,
  .bot_arrow      = 0xffffffff,
  .comm           = 0xffffff66, 
};

ColorScheme brightColors = {
  .background     = 0xffffffff,
  .text           = 0x000000ff,
  .bot_border     = 0x333333ff,
  .bot_left_leg   = 0x770000ff,
  .bot_right_leg  = 0x007700ff,
  .bot_line_front = 0x0000ffff,
  .bot_arrow      = 0x000000ff,
  .comm           = 0x00000066, 
};

ColorScheme *colorscheme = &darkColors;



int quit = 0;

int c_x = 0;
int c_y = 0;
float display_scale = 1;  
int display_w = 1200;
int display_h = 800;
int saveVideo = 0;  //if non-zero, save screenshot every Nth frame

void dieSDL(char *reason)
{
  fprintf(stderr, reason, SDL_GetError());
  exit (1);
}


kilobot *grabbed = NULL; // the kilobot currently picked up with the mouse, or NULL
kilobot *grabbedRot = NULL; // the kilobot currently picked for rotation, or NULL
double angle; //variables for rotation
int rotX0;

/* find a bot close to the screen coordinates (x, y)
 * return it's index, or -1 if no bot was found
 */
int find_bot_index (int x, int y)
{
  int RR = allbots[0]->radius * display_scale;
  RR *= RR; // radius squared

  int i;
  for (i = 0; i < n_bots; i++)
    {
      int dx = allbots[i]->screen_x - x;
      int dy = allbots[i]->screen_y - y;
      int rr = dx*dx + dy*dy;
      if (rr < RR)
	return i;
    }
  return -1;
}

/* find a bot close to the screen coordinates (x, y)
 * return a kilobot* to it, or NULL if no bot was found
 */

kilobot * find_bot (int x, int y)
{
  int i =  find_bot_index (x, y);
  if (i >= 0)
    return allbots[i];
  else
    return NULL;
}


void move_bot_to_mouse(kilobot *bot)
{
  int x, y;
  SDL_GetMouseState (&x, &y);
  bot->x = (x - display_w/2) / display_scale + c_x;
  bot->y = (y - display_h/2) / display_scale + c_y;
}

/* try to grab a bot with the mouse*/
void grab_bot(int x, int y)
{
  grabbed = find_bot (x, y);
  if (grabbed)
    move_bot_to_mouse(grabbed);
}


void rotateBot (kilobot *bot)
{
  int x, y;
  SDL_GetMouseState (&x, &y);
  
  bot->direction = angle + (x-rotX0)*.05;
}

void grab_bot_rot(int x, int y)
{
  grabbedRot = find_bot (x, y);
  if (grabbedRot)
    {
      rotX0 = x;
      angle = grabbedRot->direction;
    }
}

void release_bot()
{
  grabbed = NULL;
  grabbedRot = NULL;
}

//try to open a file for reading. Return 1 if successful.
int fileExists (char *fileName)
{
  FILE *f;
  f = fopen (fileName, "rb");
  if (f == NULL)
    return 0;
  fclose (f);
  return 1;
}

void screenshot (SDL_Surface *s)
{
  static int screenshotN = 0;

  if (screenshotN == 0) //at the first screenshot, make directory
    mkdir ("screenshots", 0777);
  // windows: use _mkdir ("screenshots");
  
  char fileName[200];
  const char *bot_name = get_string_param("botName", "default");
  while(1)
    {
      sprintf (fileName, "screenshots/%s%03d.bmp", bot_name, screenshotN);
      if (!fileExists(fileName))
	break;
      screenshotN++;
    }
  //fileName now contains the first non-existing file name

  if (SDL_SaveBMP(s, fileName))
    dieSDL ("Could not save screenshot.\n");
  else
    printf ("Screenshot %s\n", fileName);
}



void input(void)
{
  static int save_tx_period_ticks = 1;
 
  int i;
  SDL_Event event;      
  while (SDL_PollEvent(&event)) 
    switch(event.type) 
      {
      case SDL_QUIT:
	quit = 1;
	break;
      case SDL_KEYDOWN:
	switch( event.key.keysym.sym )
	  {
	  case SDLK_ESCAPE:  
	    quit = 1;           
	    break;
	  case SDLK_s:
	    screenshot(screen);
	    break;
	  case SDLK_v:
	    saveVideo = !saveVideo;
	    break;
	  case SDLK_SPACE:
	    if (state==RUNNING)
	      state = PAUSE;
	    else if (state == PAUSE)
	      state = RUNNING;
	    break;
	  case SDLK_a:    //add bot
	    break;
	  case SDLK_F5: // "userdefined" callback to the bot program - (reread JSON)
	    if (callback_F5)
	      callback_F5();
	    break;
	  case SDLK_F6: // "userdefined" callback to the bot program - (restart GRN)
	    if (callback_F6)
	      for (i = 0; i < n_bots; i++)
		{
		  current_bot = i;      // for Me() to return the right bot
		  mydata = Me()->data;
		  kilo_uid = i;         // XXX careful...
		  callback_F6();
		}
	    break;
	  case SDLK_F11: //simulate at maximum speed (no delay between frames)
	    fullSpeed = !fullSpeed;
	    break;
	  case SDLK_F12: // fast communication mode
	    if (tx_period_ticks > 1)
	      {
		save_tx_period_ticks = tx_period_ticks;
		tx_period_ticks = 1;
	      }
	    else
	      	tx_period_ticks = save_tx_period_ticks;
	    break;
	  default: break;
	  }
	break;
      case SDL_MOUSEBUTTONDOWN:
	if (event.button.button == SDL_BUTTON_LEFT )
	  grab_bot(event.button.x, event.button.y);
	else if (event.button.button == SDL_BUTTON_RIGHT)
	  grab_bot_rot(event.button.x, event.button.y);
	break;
      case SDL_MOUSEBUTTONUP:
	release_bot();
	break;
      default: break;
      }

    if (grabbed)
      move_bot_to_mouse(grabbed);
    if (grabbedRot)
      rotateBot(grabbedRot);

  // Check which keys are held down
  // these are for keys that should have an effect as long as the key is held down
  // for once/keypress events, use the switch above.
  Uint8 *keystates = SDL_GetKeyState(NULL);
  
  double s = 1.05; // magnification factor per frame
  int d = 20/display_scale;      // amount to move per frame
  if (d < 1)
    d = 1;
  
  if (keystates[SDLK_KP_PLUS] || keystates[SDLK_PLUS])
   display_scale *= s;
 if (keystates[SDLK_KP_MINUS] || keystates[SDLK_MINUS])
   display_scale *= 1/s;
 if (keystates[SDLK_RIGHT])
   c_x += d;
 if (keystates[SDLK_LEFT])
   c_x -= d; 
 if (keystates[SDLK_UP])
   c_y -= d; 
 if (keystates[SDLK_DOWN])
   c_y += d;
 if (keystates[SDLK_KP_MULTIPLY]) // faster
   stepsPerFrame++;
 if (keystates[SDLK_KP_DIVIDE])   // slower
   if (stepsPerFrame > 0)
     stepsPerFrame--;
 if (keystates[SDLK_F1])
   spread_out(500);
 if (keystates[SDLK_F2])
   {
     spread_out(-200);
     update_interactions(n_bots);
   }
}

SDL_Surface *makeWindow(int width, int height)
{
  SDL_Surface *screen;

  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0)  {
    dieSDL("SDL init failed: %s\n"); 
  }

  const SDL_VideoInfo *info = SDL_GetVideoInfo();

  //printf("Screen %d x %d\n", info->current_w, info->current_h);

  // if an absolute size is specieied, use it
  display_w = get_int_param("displayWidth",  -1);
  display_h = get_int_param("displayHeight", -1);
  display_scale = get_float_param("displayScale", 1.0);

  // if no absolute size was given, try a relative one
  if (display_w <= 0 && display_h <= 0)
    {
      float d_w_percent = get_float_param("displayWidthPercent",  0.9);
      float d_h_percent = get_float_param("displayHeightPercent", 0.9);

      display_w = info->current_w * d_w_percent;
      display_h = info->current_h * d_h_percent;
    }
  
    
  const char *bot_name = get_string_param("botName", "default");
  SDL_WM_SetCaption(bot_name, bot_name);

  screen = SDL_SetVideoMode(display_w, display_h, 32,
			    SDL_SWSURFACE|SDL_DOUBLEBUF);

  if (screen == NULL) dieSDL("SDL_SetVideoMode failed: %s\n"); // FIXME: Hmmm

  // set font
  gfxPrimitivesSetFont (font, 8, 8);
  
  return screen;
}

Uint32 conv_RGBA(int r, int g, int b, int a)
{
  return a + (b << 8) + (g << 16) + (r << 24);
}

void draw_commLines(SDL_Surface *surface)
{
  int i;
  for (i = 0; i < NcommLines; i++)
    {
      kilobot *from = commLines[i].from;
      kilobot *to   = commLines[i].to;

      int x1 = display_w/2 + display_scale * (from->x - c_x); 
      int y1 = display_h/2 + display_scale * (from->y - c_y);
      int x2 = display_w/2 + display_scale * (to->x - c_x); 
      int y2 = display_h/2 + display_scale * (to->y - c_y);  

      lineColor (surface, x1, y1, x2, y2, colorscheme->comm);
    }
}

/* Draw history */
void draw_bot_history(SDL_Surface *surface, int w, int h, kilobot *bot)
{
  int HLEN = 2000; // number of history points to draw
  if (get_int_param("showHist", 0)) {
    float i_alpha = 255.;
    for (int i=bot->p_hist-1; i>=bot->p_hist-HLEN && i >= 0; i--) {
      Uint32 ui_color = conv_RGBA(85 * bot->r_led, 85 * bot->g_led, 85 * bot->b_led, (int) i_alpha);
      i_alpha = i_alpha * (1-3.0/HLEN);
      filledCircleColor(surface,
			display_w/2 + display_scale * (bot->x_history[i] - c_x),
			display_h/2 + display_scale * (bot->y_history[i] - c_y),
			display_scale * 2, ui_color);
    }
  }
  
}

void draw_bot(SDL_Surface *surface, int w, int h, kilobot *bot)
{
  int r = bot->radius;

  /* Draw the bot's body */

  int draw_x = w/2 + display_scale * (bot->x - c_x);
  int draw_y = h/2 + display_scale * (bot->y - c_y);
  
  bot->screen_x = draw_x;
  bot->screen_y = draw_y;
  int rBody = display_scale * r;
  filledCircleColor(surface, draw_x, draw_y, rBody, colorscheme->bot_border);


  /* Draw line to front */
  int x_front = draw_x + display_scale * r * sin(bot->direction);
  int y_front = draw_y + display_scale * r * cos(bot->direction);
  lineColor(screen, draw_x, draw_y, x_front, y_front, colorscheme->bot_line_front);

  
  /* Draw legs */
  //int x_l = draw_x - display_scale * r * cos(bot->direction);
  //int y_l = draw_y + display_scale * r * sin(bot->direction);
  int x_l = draw_x + display_scale * r * sin(bot->direction + bot->leg_angle);
  int y_l = draw_y + display_scale * r * cos(bot->direction + bot->leg_angle);
  filledCircleColor(surface, x_l, y_l, display_scale * 2, colorscheme->bot_left_leg);

  //int x_r = draw_x + display_scale * r * cos(bot->direction);
  //int y_r = draw_y - display_scale * r * sin(bot->direction);
  int x_r = draw_x + display_scale * r * sin(bot->direction - bot->leg_angle);
  int y_r = draw_y + display_scale * r * cos(bot->direction - bot->leg_angle);
  filledCircleColor(surface, x_r, y_r, display_scale * 2, colorscheme->bot_right_leg);

  /* Draw LED */
  Uint32 led_color = conv_RGBA(85 * bot->r_led, 85 * bot->g_led, 85 * bot->b_led, 255);
  int rLED = display_scale * r * 0.9;
  // don't allow the LED to cover the body circle completely
  if (rLED >= rBody)
    rLED = rBody - 1;
  // don't allow the LED to vanish either
  if (rLED < 1)
    rLED = 1;
  filledCircleColor(surface, draw_x, draw_y, rLED, led_color);

  
  /* Draw a triangle pointing forward */
  int txf = draw_x + display_scale * r*.4 * sin(bot->direction);
  int tyf = draw_y + display_scale * r*.4 * cos(bot->direction);
  int tx1 = draw_x + display_scale * r*.4 * sin(bot->direction+2*M_PI*.4);
  int ty1 = draw_y + display_scale * r*.4 * cos(bot->direction+2*M_PI*.4);
  int tx2 = draw_x + display_scale * r*.4 * sin(bot->direction-2*M_PI*.4);
  int ty2 = draw_y + display_scale * r*.4 * cos(bot->direction-2*M_PI*.4);
  
  filledTrigonColor (screen, txf, tyf, tx1, ty1, tx2, ty2, colorscheme->bot_arrow);
  

  /* Draw transmit radius */
  if (get_int_param("showCommsRadius", 1)) {
    if (bot->tx_enabled == 1) {
      circleColor(surface, draw_x, draw_y, display_scale * bot->cr, colorscheme->comm);
    }
  }

}

char botinfo_buffer[100];

//default callback_botinfo function
char *botinfo_simple()
{
  sprintf (botinfo_buffer, "%d", kilo_uid);
  return botinfo_buffer;
}

void displayString (SDL_Surface *surface, int x, int y, char *s)
{
  int done = 0;
  int i;

  do {
    i = 0;
    while (s[i] != '\n' && s[i] != 0)
      i++;
    if (s[i] == 0)
      done = 1;
    s[i] = 0;
    
    stringColor (surface, x, y, s, colorscheme->text);
    s += i+1;
    y += 10;
  } while (!done);
}


// draw status message for one bot, if the mouse is over it
// also draw a simulator status
void draw_status(SDL_Surface *surface, int w, int h, double FPS)
{
  char buf[100];
  sprintf(buf, "%3dx  %8d  %5.1f FPS", stepsPerFrame, kilo_ticks, FPS);
  displayString(surface, 10, 2, buf);
  
  
  // find bot under cursor
  int x, y;
  SDL_GetMouseState (&x, &y);

  int i = find_bot_index (x, y);
  if (i >= 0)
    {
      //switch to this bot
      current_bot = i;      // for Me() to return the right bot
      mydata = Me()->data;
      kilo_uid = i;         // XXX careful...
      
      char *s = callback_botinfo();
      displayString(surface, 10, h-30, s);
    }
}





