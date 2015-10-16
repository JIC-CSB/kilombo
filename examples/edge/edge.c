/* Kilobot Edge following demo
 * 
 * Ivica Slavkov, Fredrik Jansson  2015
 */

#include <math.h>

#include <kilombo.h>

#include "edge.h"

enum {STOP,LEFT,RIGHT,STRAIGHT};

typedef struct
{
  Neighbor_t neighbors[MAXN];

  int N_Neighbors;
  uint8_t bot_type;
  uint8_t bot_state;
  uint8_t move_type;

  message_t transmit_msg;
  char message_lock;

  received_message_t RXBuffer[RB_SIZE];
  uint8_t RXHead, RXTail;

} MyUserdata;

REGISTER_USERDATA(MyUserdata)

#ifdef SIMULATOR
#include <stdio.h>    // for printf
#else
#define DEBUG         // for printf to serial port
#include "debug.h"
#endif


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


// message rx callback function. Pushes message to ring buffer.
void rxbuffer_push(message_t *msg, distance_measurement_t *dist) {
    received_message_t *rmsg = &RB_back();
    rmsg->msg = *msg;
    rmsg->dist = *dist;
    RB_pushback();
}

message_t *message_tx()
{
  if (mydata->message_lock)
    return 0;
  return &mydata->transmit_msg;
}

void set_bot_state(int state)
{
  mydata->bot_state = state;
}

int get_bot_state(void)
{
  return mydata->bot_state;
}

void set_move_type(int type)
{
  mydata->move_type = type;
}

int get_move_type(void)
{
  return mydata->move_type;
}

/* Process a received message at the front of the ring buffer.
 * Go through the list of neighbors. If the message is from a bot
 * already in the list, update the information, otherwise
 * add a new entry in the list
 */

void process_message()
{
  uint8_t i;
  uint16_t ID;

  uint8_t *data = RB_front().msg.data;
  ID = data[0] | (data[1] << 8);
  uint8_t d = estimate_distance(&RB_front().dist);

  // search the neighbor list by ID
  for (i = 0; i < mydata->N_Neighbors; i++)
    if (mydata->neighbors[i].ID == ID)
      {// found it
    	break;
      }

  if (i == mydata->N_Neighbors){  // this neighbor is not in list
    if (mydata->N_Neighbors < MAXN-1) // if we have too many neighbors,
      mydata->N_Neighbors++;          // we overwrite the last entry
                                      // sloppy but better than overflow
  }

  // i now points to where this message should be stored
  mydata->neighbors[i].ID = ID;
  mydata->neighbors[i].timestamp = kilo_ticks;
  mydata->neighbors[i].dist = d;
  mydata->neighbors[i].N_Neighbors = data[2];
  mydata->neighbors[i].n_bot_state = data[3];
}

/* Go through the list of neighbors, remove entries older than a threshold,
 * currently 2 seconds.
 */
void purgeNeighbors(void)
{
  int8_t i;

  for (i = mydata->N_Neighbors-1; i >= 0; i--)
    if (kilo_ticks - mydata->neighbors[i].timestamp  > 64) //32 ticks = 1 s
      { //this one is too old.
	mydata->neighbors[i] = mydata->neighbors[mydata->N_Neighbors-1];
	//replace it by the last entry
	mydata->N_Neighbors--;
      }
}

void setup_message(void)
{
  mydata->message_lock = 1;  //don't transmit while we are forming the message
  mydata->transmit_msg.type = NORMAL;
  mydata->transmit_msg.data[0] = kilo_uid & 0xff;     // 0 low  ID
  mydata->transmit_msg.data[1] = kilo_uid >> 8;       // 1 high ID
  mydata->transmit_msg.data[2] = mydata->N_Neighbors; // 2 number of neighbors
  mydata->transmit_msg.data[3] = get_bot_state();     // 3 bot state

  mydata->transmit_msg.crc = message_crc(&mydata->transmit_msg);
  mydata->message_lock = 0;
}

void setup()
{
  rand_seed(kilo_uid + 1); //seed the random number generator
  
  mydata->message_lock = 0;

  mydata->N_Neighbors = 0;
  set_move_type(STOP);
  set_bot_state(LISTEN);

  setup_message();
}

void receive_inputs()
{
  while (!RB_empty())
    {
      process_message();
      RB_popfront();
    }
  purgeNeighbors();
}

uint8_t get_dist_by_ID(uint16_t bot)
{
  uint8_t i;
  uint8_t dist = 255;
  
  for(i = 0; i < mydata->N_Neighbors; i++)
    {
      if(mydata->neighbors[i].ID == bot)
	{
	  dist = mydata->neighbors[i].dist;
	  break;
	}
    }
  return dist;
}

uint8_t find_nearest_N_dist()
{
  uint8_t i;
  uint8_t dist = 90;

  for(i = 0; i < mydata->N_Neighbors; i++)
    {
      if(mydata->neighbors[i].dist < dist)
	{
	  dist = mydata->neighbors[i].dist;
	}
    }
  return dist;
}

void follow_edge()
{
  uint8_t desired_dist = 55;
  if(find_nearest_N_dist() > desired_dist)
    {
      if(get_move_type() == LEFT)
	spinup_motors();
      set_motors(0, kilo_turn_right);
      set_move_type(RIGHT);
    }

  //if(find_nearest_N_dist() < desired_dist)
  else
    {
      if(get_move_type() == RIGHT)
	spinup_motors();
      set_motors(kilo_turn_left, 0);
      set_move_type(LEFT);
    }
}

void loop()
{
  //receive messages
  receive_inputs();
  if(kilo_uid == 0)
    {
      set_color(RGB(3,0,0));
      follow_edge();
    }
  
  setup_message();
}

extern char* (*callback_botinfo) (void);
char *botinfo(void);

int main(void)
{
  kilo_init();

#ifdef DEBUG
  // setup debugging, i.e. printf to serial port, in real Kilobot
  debug_init();
#endif

  SET_CALLBACK(botinfo, botinfo);
  SET_CALLBACK(reset, setup);

  RB_init();                       // initialize ring buffer
  kilo_message_rx = rxbuffer_push;
  kilo_message_tx = message_tx;    // register our transmission function

  kilo_start(setup, loop);

  return 0;
}

#ifdef SIMULATOR
// provide a text string for the status bar, about this bot
static char botinfo_buffer[10000];
char *botinfo(void)
{
  int n;
  char *p = botinfo_buffer;
  n = sprintf (p, "ID: %d ", kilo_uid);
  p += n;

  n = sprintf (p, "Ns: %d, dist: %d\n ", mydata->N_Neighbors, find_nearest_N_dist());
  p += n;

  return botinfo_buffer;
}
#endif
