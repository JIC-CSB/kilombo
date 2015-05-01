#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <SDL/SDL.h>

#include "skilobot.h"

SDL_Surface *makeWindow(int width, int height);
void input(void);

void draw_bot(SDL_Surface *surface, int w, int h, kilobot *bot);
void draw_bot_history(SDL_Surface *surface, int w, int h, kilobot *bot);
void draw_commLines(SDL_Surface *surface);
void draw_status(SDL_Surface *surface, int w, int h, double FPS);

extern SDL_Surface *screen;
extern int quit;
extern int sim_pause;
extern int display_w;
extern int display_h;
extern float display_scale;
extern int saveVideo;

#endif // _DISPLAY_H
