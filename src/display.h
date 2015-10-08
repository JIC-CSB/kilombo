#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <SDL/SDL.h>
#include "skilobot.h"

// Declaration of color schemes.
typedef struct {
  Uint32 background;
  Uint32 text;
  Uint32 bot_border;
  Uint32 bot_left_leg;
  Uint32 bot_right_leg;
  Uint32 bot_arrow;
  Uint32 bot_line_front;
  Uint32 comm;
  double LEDa, LEDb;
  int anti_alias;
} ColorScheme;
// SDL_GFX uses Uint32 for colors, format: 0xRRGGBBAA. alpha = 0xff is opaque

void init_SDL(void);
SDL_Surface *makeWindow(void);
SDL_Surface * makeSurface(void);
void input(void);

void draw_bot(SDL_Surface *surface, int w, int h, kilobot *bot);
void draw_bot_history(SDL_Surface *surface, int w, int h, kilobot *bot);
void draw_bot_history_ring(SDL_Surface *surface, int w, int h, kilobot *bot);
void draw_commLines(SDL_Surface *surface);
void draw_status(SDL_Surface *surface, int w, int h, double time, double FPS);
void set_display_center(double X, double Y);

extern ColorScheme *colorscheme;
extern ColorScheme darkColors, brightColors;

extern SDL_Surface *screen;
extern int quit;
extern int sim_pause;



#endif // _DISPLAY_H
