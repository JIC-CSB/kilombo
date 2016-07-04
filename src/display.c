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

// note! background is different from the rest, in the format 0x00RRGGBB
ColorScheme darkColors = {
  .background     = 0x00000000,
  .text           = 0xffffffff,
  .bot_border     = 0xffffffff,
  .bot_left_leg   = 0xffffffff,
  .bot_right_leg  = 0xffffffff,
  .bot_line_front = 0x0000ffff,
  .bot_arrow      = 0xffffffff,
  .comm           = 0xffffff66, 
  .LEDa           = 0,
  .LEDb           = 85,
  .anti_alias      = 0,
};

ColorScheme brightColors = {
  .background     = 0xffffffff,
  .text           = 0x000000ff,
  .bot_border     = 0x000000ff,
  .bot_left_leg   = 0x000000ff,
  .bot_right_leg  = 0x000000ff,
  .bot_line_front = 0x000000ff, // not visible, same color as outline
  .bot_arrow      = 0x000000ff,
  .comm           = 0x000000ff,
  .LEDa           = 63,
  .LEDb           = 64,
  .anti_alias     = 1
};

ColorScheme *colorscheme = &darkColors;


int quit = 0;

double c_x = 0;
double c_y = 0;

void set_display_center(double X, double Y)
{
  c_x = X;
  c_y = Y;
}

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
  int RR = allbots[0]->radius * simparams->display_scale;
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
  bot->x = (x - simparams->display_w/2) / simparams->display_scale + c_x;
  bot->y = (y - simparams->display_h/2) / simparams->display_scale + c_y;
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

  while(1)
    {
      sprintf (fileName, "screenshots/%s%03d.bmp", simparams->bot_name, screenshotN);
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
	    simparams->saveVideo = !simparams->saveVideo;
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
		  prepare_bot(allbots[i]);
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
  int d = 20/simparams->display_scale;      // amount to move per frame
  if (d < 1)
    d = 1;
  
  if (keystates[SDLK_KP_PLUS] || keystates[SDLK_PLUS] || keystates[SDLK_p])
    {
      simparams->display_scale *= s;
      printf ("Scale: %f\n", simparams->display_scale);
    }
  if (keystates[SDLK_KP_MINUS] || keystates[SDLK_MINUS])
    {
      simparams->display_scale *= 1/s;
      printf ("Scale: %f\n", simparams->display_scale);
    }
  if (keystates[SDLK_RIGHT])
   c_x += d;
 if (keystates[SDLK_LEFT])
   c_x -= d; 
 if (keystates[SDLK_UP])
   c_y -= d; 
 if (keystates[SDLK_DOWN])
   c_y += d;
 if (keystates[SDLK_KP_MULTIPLY] ||
     keystates[SDLK_F4] ) // faster
   simparams->stepsPerFrame++;
 if (keystates[SDLK_KP_DIVIDE] ||
     keystates[SDLK_F3] )   // slower
   if (simparams->stepsPerFrame > 0)
     simparams->stepsPerFrame--;
 if (keystates[SDLK_F1])
   spread_out(n_bots, 500);
 if (keystates[SDLK_F2])
   {
     spread_out(n_bots, -200);
     update_interactions(n_bots);
   }
}


void init_SDL(void)
{
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0)  {
    dieSDL("SDL init failed: %s\n"); 
  }

  // set font
  gfxPrimitivesSetFont (font, 8, 8);
}

/* ***   Icon Loading Stuff   *** */


// the following is not in use due to difficulties with transparency 

// include bitmap data for the icon in PNM (P6) format 
//#include "icon2-48px.h"

/* make an SDL surface from PNM (P6) data. Used for the program icon.
 * Complication: the image stored does not have an alpha channel.
 * The icon needs a transparent background to look nice.
 * Setting a colorkey for transparency doesn't semm to work.
 * Workaround: blit the icon to another surface which has an 
 * an alpha channel, this works.
 */
SDL_Surface* surf_from_pnm(char *data)
{
  int x, y, maxval, n;
  char *img;
  if (strncmp(data, "P6\n", 3))
    { 
      printf("Unknown format in surf_from_pnm\n");
      return NULL;
    }
  char header[5];
  
  sscanf (data, "%s %d %d %d %n", header, &x, &y, &maxval, &n);
  // n  contains the number of characters used
  // white space after last %d is important
  
  //printf ("image is %dx%d. header is %s. maxval is %d\n",
  //x, y, header, maxval);

  // find start of the raw data
  img = data + n;
  
  int rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;
#else
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = 0xff000000;
#endif
    SDL_Surface *surface =
    SDL_CreateRGBSurfaceFrom(img, x, y, 24, x*3, rmask, gmask, bmask, 0);
  
  // create second surface, with alpha channel
  SDL_Surface *surface2 = SDL_CreateRGBSurface(SDL_SWSURFACE,x,y,32, rmask, gmask, bmask, amask);
  SDL_FillRect(surface2, NULL, 0); // fill with transparent color
  
  // make the black color transparent - doesn't
  // work directly as icon, but works if first blitted to another surface.
  SDL_SetColorKey(surface, SDL_SRCCOLORKEY, 0);
  SDL_BlitSurface(surface, NULL, surface2, NULL);

  SDL_FreeSurface(surface);
  return surface2;
}

#include "icon.h"
SDL_Surface *makeIcon(void)
{
  int rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;
#else
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = 0xff000000;
#endif
  SDL_Surface *surface =
    SDL_CreateRGBSurfaceFrom(icon_data, icon_w, icon_h, 32, icon_w*4, rmask, gmask, bmask, amask);
  return surface;
}

SDL_Surface *makeWindow(void)
{
  SDL_Surface *screen;
  
  const SDL_VideoInfo *info = SDL_GetVideoInfo();

  //printf("Screen %d x %d\n", info->current_w, info->current_h);

  // if an absolute size is specieied, use it
  // if no absolute size was given, try a relative one
  if (simparams->display_w <= 0 && simparams->display_h <= 0)
    {
      simparams->display_w = info->current_w * simparams->displayWidthPercent;
      simparams->display_h = info->current_h * simparams->displayHeightPercent;
    }
  
    
  SDL_WM_SetCaption(simparams->bot_name, simparams->bot_name);

  screen = SDL_SetVideoMode(simparams->display_w, simparams->display_h, 32,
			    SDL_SWSURFACE|SDL_DOUBLEBUF);

  if (screen == NULL) dieSDL("SDL_SetVideoMode failed: %s\n"); // FIXME: Hmmm

  SDL_Surface *icon; // = SDL_LoadBMP("icon2-24px.bmp");
  //icon = surf_from_pnm((char*)MagickImage);
  icon = makeIcon();
  if (icon)
    SDL_WM_SetIcon(icon, NULL);
  else
    printf("No icon found\n");
  
  return screen;
}


/* Create an SDL surface. 
 * from SDL docs example: https://wiki.libsdl.org/SDL_CreateRGBSurface
 */
SDL_Surface * makeSurface(void)
{
  SDL_Surface *surface;
  Uint32 rmask, gmask, bmask, amask;

  /* SDL interprets each pixel as a 32-bit number, so our masks must depend
     on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    int width  = simparams->display_w;
    int height = simparams->display_h;

    if (simparams->display_w <= 0 && simparams->display_h <= 0)
      {
	return NULL;
	//printf ("When running without GUI, specify an absolute size of the images\n");
      }
    
    surface = SDL_CreateRGBSurface(0, width, height, 32,
                                   rmask, gmask, bmask, amask);
    if(surface == NULL) 
      dieSDL ("CreateRGBSurface failed: %s\n");
    return surface;
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

      int x1 = simparams->display_w/2 + simparams->display_scale * (from->x - c_x); 
      int y1 = simparams->display_h/2 + simparams->display_scale * (from->y - c_y);
      int x2 = simparams->display_w/2 + simparams->display_scale * (to->x - c_x); 
      int y2 = simparams->display_h/2 + simparams->display_scale * (to->y - c_y);  

      if (colorscheme->anti_alias)
	aalineColor (surface, x1, y1, x2, y2, colorscheme->comm);
      else
	lineColor (surface, x1, y1, x2, y2, colorscheme->comm);
    }
}

Uint32 LEDcolor (kilobot *bot, float i_alpha)
{
  //Uint32 ui_color = conv_RGBA(85 * bot->r_led, 85 * bot->g_led, 85 * bot->b_led, (int) i_alpha);
  double a = colorscheme->LEDa;
  double b = colorscheme->LEDb;
  Uint32 ui_color = conv_RGBA(a + b * bot->r_led, a + b * bot->g_led, a + b * bot->b_led, (int) i_alpha);
  return ui_color;
}

/* Draw history */
void draw_bot_history(SDL_Surface *surface, int w, int h, kilobot *bot)
{
  if (simparams->showHist) {
    float i_alpha = 255.;
    for (int i=bot->p_hist-1; i>=bot->p_hist-simparams->histLength && i >= 0; i--) {
      //Uint32 ui_color = conv_RGBA(85 * bot->r_led, 85 * bot->g_led, 85 * bot->b_led, (int) i_alpha);
      Uint32 ui_color = LEDcolor(bot, i_alpha);
      i_alpha = i_alpha * (1-3.0/simparams->histLength);
      filledCircleColor(surface,
			simparams->display_w/2 + simparams->display_scale * (bot->x_history[i] - c_x),
			simparams->display_h/2 + simparams->display_scale * (bot->y_history[i] - c_y),
			simparams->display_scale * 2, ui_color);
    }
  }
}

/* Draw history using a ring buffer of length simparams->histLength */
void draw_bot_history_ring(SDL_Surface *surface, int w, int h, kilobot *bot)
{
  if (simparams->showHist) {
    float i_alpha = 255.;
    //    for (int i=bot->p_hist-1; i!=bot->p_hist; i = (i-1+simparams->histLength)%simparams->histLength)

    // p_hist is ONE PAST the most recently stored point in the ring buffer
    // l_hist is the number of history points so far
    // n_hist is the ring buffer size
    for (int ii=1; ii < bot->l_hist && ii < bot->n_hist; ii++)
      {
	int i = (bot->p_hist - ii + simparams->histLength) % simparams->histLength;

	//Uint32 ui_color = conv_RGBA(85 * bot->r_led, 85 * bot->g_led, 85 * bot->b_led, (int) i_alpha);
	Uint32 ui_color = LEDcolor(bot, i_alpha);
	i_alpha = i_alpha * (1-3.0/simparams->histLength);
	filledCircleColor(surface,
			  simparams->display_w/2 + simparams->display_scale * (bot->x_history[i] - c_x),
			  simparams->display_h/2 + simparams->display_scale * (bot->y_history[i] - c_y),
			  simparams->display_scale * 2, ui_color);
      }
  }
}



/* Draw the kilobots.
 * 
 * Experimental anti-alias support can be enabled by setting .anti_alias = 1 in the color scheme.
 * Antialiasing is done using the aa-primitives from the gfx_library.
 * Unfortunately, filled circles and triangles are not implemented, so antialiasing for those
 * is done by drawing a non-filled, anti-aliased primitive before the filled, regular one.
 */
void draw_bot(SDL_Surface *surface, int w, int h, kilobot *bot)
{
  int r = bot->radius;
  float scale = simparams->display_scale;
 
  /* Draw the bot's body */
    
  int draw_x = w/2 + scale * (bot->x - c_x);
  int draw_y = h/2 + scale * (bot->y - c_y);
  
  bot->screen_x = draw_x;
  bot->screen_y = draw_y;
  int rBody = scale * r;

  if (colorscheme->anti_alias)
    aacircleColor(surface, draw_x, draw_y, rBody, colorscheme->bot_border);
  filledCircleColor(surface, draw_x, draw_y, rBody, colorscheme->bot_border);


  /* Draw line to front */
  int x_front = draw_x + scale * r * sin(bot->direction);
  int y_front = draw_y + scale * r * cos(bot->direction);
  lineColor(screen, draw_x, draw_y, x_front, y_front, colorscheme->bot_line_front);

  
  /* Draw legs */
  //int x_l = draw_x - scale * r * cos(bot->direction);
  //int y_l = draw_y + scale * r * sin(bot->direction);
  int x_l = draw_x + scale * r * sin(bot->direction + bot->leg_angle);
  int y_l = draw_y + scale * r * cos(bot->direction + bot->leg_angle);
    
  //int x_r = draw_x + scale * r * cos(bot->direction);
  //int y_r = draw_y - scale * r * sin(bot->direction);
  int x_r = draw_x + scale * r * sin(bot->direction - bot->leg_angle);
  int y_r = draw_y + scale * r * cos(bot->direction - bot->leg_angle);
  if (colorscheme->anti_alias && scale > 1) // for smaller scales it draws weird legs 
    {
      aacircleColor(surface, x_r, y_r, scale * 2, colorscheme->bot_right_leg);
      aacircleColor(surface, x_l, y_l, scale * 2, colorscheme->bot_left_leg);
    }
  
  filledCircleColor(surface, x_l, y_l, scale * 2, colorscheme->bot_left_leg);
  filledCircleColor(surface, x_r, y_r, scale * 2, colorscheme->bot_right_leg);

  /* Draw LED */
  //  Uint32 led_color = conv_RGBA(85 * bot->r_led, 85 * bot->g_led, 85 * bot->b_led, 255);
  Uint32 led_color = LEDcolor(bot, 255);
  int rLED = scale * r * 0.9;
  // don't allow the LED to cover the body circle completely
  if (rLED >= rBody)
    rLED = rBody - 1;
  // don't allow the LED to vanish either
  if (rLED < 1)
    rLED = 1;

  if (colorscheme->anti_alias)
    aacircleColor(surface, draw_x, draw_y, rLED, led_color);
  filledCircleColor(surface, draw_x, draw_y, rLED, led_color);

  
  /* Draw a triangle pointing forward */
  int txf = draw_x + scale * r*.4 * sin(bot->direction);
  int tyf = draw_y + scale * r*.4 * cos(bot->direction);
  int tx1 = draw_x + scale * r*.4 * sin(bot->direction+2*M_PI*.4);
  int ty1 = draw_y + scale * r*.4 * cos(bot->direction+2*M_PI*.4);
  int tx2 = draw_x + scale * r*.4 * sin(bot->direction-2*M_PI*.4);
  int ty2 = draw_y + scale * r*.4 * cos(bot->direction-2*M_PI*.4);

  if (colorscheme->anti_alias)
    aatrigonColor (screen, txf, tyf, tx1, ty1, tx2, ty2, colorscheme->bot_arrow);
  filledTrigonColor (screen, txf, tyf, tx1, ty1, tx2, ty2, colorscheme->bot_arrow);
  

  /* Draw transmit radius */
  if (simparams->showCommsRadius) {
    if (bot->tx_enabled == 1) {
      if (colorscheme->anti_alias)
	aacircleColor(surface, draw_x, draw_y, scale * bot->cr, colorscheme->comm);
      else
	circleColor(surface, draw_x, draw_y, scale * bot->cr, colorscheme->comm);      
    }
  }

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
void draw_status(SDL_Surface *surface, int w, int h, double time, double FPS)
{
  char buf[100];
  sprintf(buf, "%3dx    time: %8.1f     kilo_ticks: %8d     FPS: %5.1f",
	  simparams->stepsPerFrame, time, kilo_ticks, FPS);
  displayString(surface, 10, 2, buf);
  
  
  // find bot under cursor
  int x, y;
  SDL_GetMouseState (&x, &y);

  int i = find_bot_index (x, y);
  if (i >= 0)
    {
      //switch to this bot
      prepare_bot(allbots[i]);
      char *s = callback_botinfo();
      displayString(surface, 10, h-30, s);
    }
}





