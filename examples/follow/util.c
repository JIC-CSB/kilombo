#ifdef KILOBOT 
#include <avr/io.h>  // for microcontroller register defs
#endif

#include <kilombo.h>

#include "util.h"


// rainbow colors
uint8_t colorNum[] = {
  RGB(2,0,1),  //0 - magenta
  RGB(3,0,0),  //1 -**red
  RGB(3,1,0),  //2 - orange
  RGB(2,2,0),  //3 - yellow
  RGB(1,3,0),  //4 - yellowish green
  RGB(0,3,0),  //5 -**green
  RGB(0,2,2),  //6 - cyan
  RGB(0,1,2),  //7 - sky blue
  RGB(0,0,3),  //8 -**blue
  RGB(1,0,2)   //9  purple
};

/*
uint8_t colorNum[] = {
  RGB(0,0,0),  //0 - off
  RGB(1,0,0),  //1 - red
  RGB(0,1,0),  //2 - green
  RGB(0,0,1),  //3 - blue
  RGB(1,1,0),  //4 - yellow
  RGB(0,1,1),  //5 - cyan
  RGB(1,0,1),  //6 - purple
  RGB(2,1,0),  //7  - orange
  RGB(1,1,1),  //8  - white
  RGB(3,3,3)   //9  - bright white
};
*/


/* A helper function for setting motor state.
 * Automatic spin-up of left/right motor, when necessary.
 * The standard way with spinup_motors() causes a small but
 * noticeable jump when a motor which is already running is spun up again.
 */

void smooth_set_motors(uint8_t ccw, uint8_t cw) {
  // OCR2A = ccw;  OCR2B = cw;  
#ifdef KILOBOT 
  uint8_t l = 0, r = 0;
  if (ccw && !OCR2A) // we want left motor on, and it's off
    l = 0xff;
  if (cw && !OCR2B)  // we want right motor on, and it's off
    r = 0xff;
  if (l || r)        // at least one motor needs spin-up
    {
      set_motors(l, r);
      delay(15);
    }
#endif

  // spin-up is done, now we set the real value
  set_motors(ccw, cw);
}

void motion(uint8_t type)
{
  switch (type)
    {
    case LEFT:
      smooth_set_motors(kilo_turn_left, 0); 
      break;
    case RIGHT:
      smooth_set_motors(0, kilo_turn_right);
      break;
    case STRAIGHT:
      smooth_set_motors(kilo_straight_left, kilo_straight_right);
      break;
    case STOP:
    default:
      smooth_set_motors(0, 0);
      break;
    }
}

float clipf(float f, float min, float max)
{
  if (f < min)
    return min;
  if (f > max)
    return max;
  return f;
}

