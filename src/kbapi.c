#include <math.h>
#include "skilobot.h"
#include "kilolib.h"

/* pointers to messaging functions 
 * the kilobot program typically sets these in main()
 *
 * for simplicity we have only ONE set of these instead of
 * one for each bot. If all bots register the same functions, 
 * this will work.
 */
message_tx_t kilo_message_tx = NULL;
message_tx_success_t kilo_message_tx_success = NULL;
message_rx_t kilo_message_rx = NULL;


/* the clock variable. Counts ticks since beginning of the program.
 * Each bot really has it's own, but for now we have just one.
 * Declared volatile to match declaration in kilolib.h
 */
volatile uint32_t kilo_ticks = 0;

// the simulator will copy a new UID here, before calling each bot
uint16_t kilo_uid = 0;

/* motor calibration values 
 * In the kilobots, these are different for each robot, and are stored in the EEPROM
 * In the simulator we don't need them to be different.
 * currently the actual movement speed is set using speed and turn_rate in the params,
 * these only need to be non-zero for the motors to be considered on.
 */
uint8_t kilo_turn_left  = 7;
uint8_t kilo_turn_right = 7;
uint8_t kilo_straight_left = 7;
uint8_t kilo_straight_right = 7;


/* Each bot is required to call kilo_init.
 * In the simulator, we initialize the software RNG.
 */
void kilo_init(void)
{
  kilobot* self = Me();
  self->seed=0xaa;        //same state as in the bot
  self->accumulator = 0;
}


/* In the kilobot, this function calls setup once.
 * Then it loops forever calling the function loop() and checks for control messages.
 * We just store the function pointers and call setup()
 *   the loop() will be run for all bots later
 */
void kilo_start(void (*setup)(void), void (*loop)(void))
{
  current_bot->user_setup = setup;
  current_bot->user_loop = loop;
}

void set_motors(uint8_t left, uint8_t right)
{
  kilobot* self = Me();

  self->left_motor_power = left;
  self->right_motor_power  = right;
  return;
}

/* run the motors at full power for 15 ms to start them
 * Ignored in the simulator.
 */
void spinup_motors()
{
}

/* Set the LED colour. Bits: 00bbggrr
 */
void set_color(uint8_t color)
{
  int r = color & 3;
  int g = (color >> 2) & 3;
  int b = (color >> 4) & 3;
  kilobot* self = Me();

  self->r_led = r;
  self->g_led = g;
  self->b_led = b;
}

/* Estimate distance from signal strength measurements.
 * We store the distance in mm in the high_gain field when passing messages,
 * so we can simply return it here.
 */
uint8_t estimate_distance(const distance_measurement_t *d)
{
  return d->high_gain;
}

// checksum for messages 
uint16_t message_crc(const message_t *msg)
{
  return 0; 
  /* if we never check it we don't need to implement it.
   * otherwise copy from kilolib, message_crc.c and
   * the crc functions from util/crc16.h 
   */
}


/* delay is tricky to implement in the simulator
 * do as much as possible using the kilo_ticks variable instead 
 */
void _delay_ms(int ms)
{
  /* TODO - implementation! */
  return;
}

// the kilobot API version of delay
void delay (uint16_t ms) 	
{
  return;
}

/* Hardware random number generator - "truly random" in the bot.
 * In the simulator, we use rand(), with the SAME generator shared by all bots
 * and shared by the simulator itself, if it uses rand somewhere internally.
 */
uint8_t rand_hard()
{
  return rand() & 0xFF;
}

/* Software random number generator.
 * this is the same as the one in kilolib.
 * Each bot has it's own seed and accumulator, so the sequence should be the same as in the bot
 */
uint8_t rand_soft()
{
  kilobot* self = Me();
  self->seed ^= self->seed<<3;
  self->seed ^= self->seed>>5;
  self->seed ^= self->accumulator++>>2;
  return self->seed;
}

void rand_seed(uint8_t seed)
{
  kilobot* self = Me();
  self->seed = seed;
}


float get_potential(int type)
{
  kilobot* self = Me();
  switch(type)
    {
    case POT_LINEAR:
      return self->x;
    case POT_PARABOLIC: 
      return pow(self->x, 2) + pow(self->y, 2);
    case POT_GRAVITY:
      return 1.0 / hypot(self->x, self->y);      
    default:
      return 0;
    }
  
}

// 10 bit measurement of ambient light
// return the x coordinate as the light intensity
// - simulates a light gradient in x
int16_t get_ambientlight()
{
  kilobot* self = Me();
  int l;

  if (user_light != NULL)
	  l = user_light(self->x, self->y);
  else
  // parabolic well
  	  l = ( pow(self->x, 2) + pow(self->y, 2) )/1000;

  // constrain to interval [0,1023]
  if (l > 1023)
    l = 1023;
  
  if (l < 0)
    l = 0;
  
  return l;
}

// 10 bit measurement of battery voltage.
// TODO: find the scale, implement a decrease with time?
int16_t get_voltage()
{
  return 876;
}

int16_t get_temperature()
{
  return 314;
}

