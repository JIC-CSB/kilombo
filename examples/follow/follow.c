/* Kilobot follow-the-leader demonstration
 * 
 * Fredrik Jansson, Johanna Grönqvist 2015
 */


#include <math.h>

#include <kilombo.h>

#include "follow.h"
#include "util.h"
#include "communication.h"


#ifndef SIMULATOR
#include <avr/io.h>      // for microcontroller register defs 
#define DEBUG          // for printf to serial port
#include "debug.h"
#endif


#define D_MIN 45   // Min distance. Wait when this close to the one we follow
#define D_MAX 65   // Max distance. Wait when our follower is this far away

#define T_CYCLE 70
#define T_MOVE 40
#define T_STRAIGHT 00

#define NO_GRADIENT 65535
#define NO_ID 65535

REGISTER_USERDATA(USERDATA)

/* A potential linear in the distance d. 
 * 0 at 100 mm and further, negative when closer.
 */
float v(uint8_t d)
{
  float v = 0;
  if (d < 100)
    v = d - 100;
  return v;
}

// Calculate the potential
float potential()
{
  float V = 0;
  
  uint8_t i;
  if (mydata->N_Neighbors == 0)
    V = 0;
  else
    {
      for (i = 0; i < mydata->N_Neighbors; i++)
	{
	  // attraction towards neighbor with number = own number - 1 
	  if (mydata->neighbors[i].gradient == mydata->gradient - 1)
	      //&& mydata->neighbors[i].full == 0)
	    V += v(mydata->neighbors[i].dist);	  
  	}

      // repel other robots ?
      for (i = 0; i < mydata->N_Neighbors; i++)
	{
	  //V += repel_pot(mydata->neighbors[i].dist);
	}
    }
  return V;
}


// extra motion rules
void extra_rules()
{
}

/* Motion rules for the leader
 * The leader moves for T_MOVE ticks, then waits so that the cycle is T_CYCLE ticks long. 
 * When moving, alternate between going straight and turning - walks in a large circle
 * or just go straight.
 */
void leader()
{
  mydata->cycle_time++;
  if (mydata->WG == WAIT)
    motion(STOP);
    else
    {
      if (mydata->cycle_time < T_MOVE)
	{
	  motion(mydata->state);
	}
      else if (mydata->cycle_time < T_CYCLE)
	{
	  motion(STOP);
	}
      else 
	{ // end of cycle 
	  mydata->turn++;
	  mydata->cycle_time = 0;
	  
	  if (mydata->turn == 100)
	    mydata->turn = 0;
	  
	   //walk in a large circle - good for simulator
	    mydata->state = STOP;
	    if (mydata->turn == 0)
	      mydata->state = LEFT;
	    else
	      mydata->state = STRAIGHT;
	  
	  
	  // just go straight
	  //mydata->state = STRAIGHT;
	}
    }
  set_color(RGB(1,1,1));

  mydata->gradient = 0;
  setup_message();
}

void idle_search()
{
   mydata->cycle_time++;
   
   if (mydata->cycle_time < mydata->movement_time)
     {
       motion(mydata->state);
     }
   else if (mydata->cycle_time < T_CYCLE)
     {
       motion(STOP);
     }
   else 
     { // end of cycle 
       mydata->turn++;
       mydata->cycle_time = 0;
       mydata->movement_time = (float)rand_soft() * T_CYCLE / 255.0;
       mydata->state = (rand_soft()&1) ? LEFT:RIGHT;
     }
}

/* Movement rules for followers
 *
 */
void movement(float V)
{
  if (mydata->gradient == NO_GRADIENT)
    { // I'm not in the chain - search
      // idle_search();
      // when starting from a chian, this search sometimes break the chain so now its off.
      return;
    }
  
  uint8_t otherstate;
  otherstate = (mydata->state==LEFT)?RIGHT:LEFT;

  if (mydata->prevWG == WAIT)
    mydata->Vmin = V;         // minimum distance this turn

  if (V < mydata->Vmin)       // potential decreased
    { 
      mydata->Vmin = V;       // store new minimum
      mydata->badsteps = 0;   // reset badsteps-counter
    }

  if (mydata->changeTicks > 0) // after changing direction, don't change back immediately
    {
      mydata->changeTicks--;
      if (mydata->changeTicks == 0)
	{
	  if (V >  mydata->Vmin)   // this step was in a bad direction. If this happens for two consecutive
	    mydata->badsteps++;    // moves, we're heading the wrong way. Just keep turning then.
	  mydata->Vmin = V;        // dead-time ended. Store current distance as minimum.
	}
    }
  else if (V > mydata->Vmin && mydata->badsteps < 1) // we passed the minimum distance. 
    {                                                
      mydata->state = otherstate;  // change direction
      mydata->Vmin = V;
      mydata->changeTicks = 80;    // length of dead time after direction change    
    }
  else if (mydata->prevWG == WAIT)
    {
    }
  
  motion(mydata->state);
}


void setup() {
  
  rand_seed(rand_hard()); //seed the random number generator
  
  mydata->N_Neighbors = 0;
  mydata->next_ticks = 0;

  mydata->next_movement_ticks = kilo_ticks;

  mydata->gradient = NO_GRADIENT;
  mydata->follower = NO_GRADIENT;
  mydata->turn = 0;
  
  // new state machine 
  
  mydata->cycle_time = 0;
  mydata->movement_time = T_MOVE; // used for idle search
  mydata->state = LEFT; 
  
  setup_message();

  mydata->WG = GO;
}



void follow_the_leader()
{
  uint8_t i;
  uint8_t d_leader = 255;
  uint8_t d_follower = 255;
  uint8_t WG_leader = WAIT, WG_follower = WAIT;
  uint16_t i_follower = NO_ID;

  // get distance to leader and follower, if they exist among neighbors.
  for (i = 0; i < mydata->N_Neighbors; i++)
    {
      if (mydata->neighbors[i].gradient == mydata->gradient-1)
	{
	  d_leader  = mydata->neighbors[i].dist;
	  WG_leader = mydata->neighbors[i].WG;
	}
      if (mydata->neighbors[i].ID == mydata->follower)
	{
	  d_follower  = mydata->neighbors[i].dist;
	  WG_follower = mydata->neighbors[i].WG;
	  i_follower = i;
	}
    }

  if (d_follower == 255)
    mydata->follower = NO_ID;  // follower not found among neighbors, forget it. I'm last now.
  if (d_leader == 255 && mydata->gradient != 0)
    {                        //lost the leader
      mydata->gradient = NO_GRADIENT;
      mydata->follower = NO_ID;
    }
 
  if (mydata->gradient == NO_GRADIENT)
    { // not part of the chain
      for (i = 0; i < mydata->N_Neighbors; i++)
	{
	  if (mydata->neighbors[i].follower == kilo_uid)
	    { // I am the chosen follower!
	      mydata->gradient = mydata->neighbors[i].gradient + 1;
	    }
	}
    }
  else
    { // I'm part of the chain
      if (mydata->follower == NO_ID)
	{ // I'm last in the chain
	  uint8_t dmin = 80; // pick followers if they are closer than this
	  for (i = 0; i < mydata->N_Neighbors; i++)
	    {  //found a neighbor without a place in the chain
	      if ((mydata->neighbors[i].gradient == NO_GRADIENT && mydata->neighbors[i].dist < dmin) ||
		   (mydata->neighbors[i].gradient == mydata->gradient+1)) //my old follower, lost and found again
		  { //pick the closest neighbor as follower
		    dmin = mydata->neighbors[i].dist;
		    mydata->follower = mydata->neighbors[i].ID;
		  }
	    }
	}
    }

  switch (mydata->WG)
    {
    case WAIT:
      if (WG_leader == WAIT && WG_follower == WAIT)  // if either does not exist, these are set to wait (above).
	{
          mydata->WG = GO;
      //break; // fall through - if too close or far, we should still wait
      //if (d_leader < D_MIN && mydata->gradient != 0) ||  (d_follower > D_MAX && mydata->follower != 255)
        }
 case GO:
      if ((d_leader < D_MIN && mydata->gradient != 0) ||    // I'm not leader, and too close to the one before me
	  (d_follower > D_MAX && mydata->follower != NO_ID) ||  // I'm not last, and my follower is too far away
	  (mydata->follower != NO_ID && kilo_ticks - mydata->neighbors[i_follower].timestamp > 16 )|| // we missed a message from the follower
	  ( WG_follower == GO) // follower is also walking. shouldn't happen but can happen due to communication errors
	  ) // we missed a message from the follower
	mydata->WG = WAIT;
      break;
    }

}

 
void gradient_propagation_follow()
{
  uint8_t i;
  uint8_t min = 255;
  uint8_t max = 0;
  uint8_t found = 0;
  
  for (i = 0; i < mydata->N_Neighbors; i++)
    {
      if (mydata->neighbors[i].gradient < min)
	min = mydata->neighbors[i].gradient;
      
      if (mydata->neighbors[i].gradient != NO_GRADIENT)
	{
	  found = 1;
	  if (mydata->neighbors[i].gradient > max)
	    {
	      max = mydata->neighbors[i].gradient;
	    }
	}
    }
  
  // mydata->gradient = min+1;
    
  if (found && max+1 < mydata->gradient)
    mydata->gradient = max+1;
  
  // avoid number collisions with neighbors
  for (i = 0; i < mydata->N_Neighbors; i++)
    {
      if(mydata->neighbors[i].gradient == mydata->gradient)  // collision
	if(mydata->neighbors[i].ID < kilo_uid) // the one with the smaller ID keeps the smaller number
	  if (mydata->gradient != NO_GRADIENT)
	    mydata->gradient++;
    }
}



// the main loop of the user program running on the bot
void loop() 
{
  // process messages in the RX ring buffer
  while (!RB_empty()) {
    process_message();
    RB_popfront();
  }
  
  // remove old entries from the neighbor table
  purgeNeighbors();

  //if (kilo_ticks > 30)  // don't look for followers before some messages have arrived
      follow_the_leader();
  
  if (kilo_ticks >= mydata->next_movement_ticks)
    {
      mydata->next_movement_ticks = kilo_ticks+1;
      //    printf ("ID: %d grad:%d\n", kilo_uid, mydata->gradient);

      if (kilo_uid == 0)
	leader();
      else
	{
	  float V = potential();

	  if (mydata->WG == GO)
	    {
	      if (mydata->prevWG == WAIT)
		mydata->cycle_time = 0;
	      movement(V);
	    }
	  else
	    motion(STOP);
	  
	  extra_rules();
	  mydata->prevWG = mydata->WG;
	}
    }
  if (mydata->gradient == 0)
    set_color(RGB(1,1,1));
  else if (mydata->gradient < NO_GRADIENT)
    set_color(colorNum[(mydata->gradient-1)%10]);
  else
    set_color(RGB(0,0,0));

    setup_message();
}

extern char* (*callback_botinfo) (void);
char *botinfo(void);

int main(void)
{
  kilo_init();

#ifdef DEBUG
  debug_init();
#endif
  
  SET_CALLBACK(botinfo, botinfo);
  SET_CALLBACK(reset, setup);
  
  //initialize ring buffer
  RB_init();
  kilo_message_rx = rxbuffer_push; 
  kilo_message_tx = message_tx;   // register our transmission function

  kilo_start(setup, loop);
  
  return 0;
}


void printNeighbors(void)
{
  uint8_t i;
  printf ("UID:%d N:%d t:%lu\n", kilo_uid, mydata->N_Neighbors, (unsigned long)kilo_ticks);
  printf ("\n");
  for (i = 0; i < mydata->N_Neighbors; i++)
    printf ("  %2d %2d %3d %lu\n", mydata->neighbors[i].ID, mydata->neighbors[i].N_Neighbors,
	    mydata->neighbors[i].dist, (unsigned long int)mydata->neighbors[i].timestamp);
  
  printf ("\n");
}

#ifdef SIMULATOR
static char botinfo_buffer[10000];
// provide a text string for the status bar, about this bot
char *botinfo(void)
{
  int i;
  char *p = botinfo_buffer;
  p+= sprintf (p, "ID: %d \n", kilo_uid);

  // p+= sprintf (p, "move: %d wait:%d turn:%d\n", mydata->move, mydata->wait, mydata->turn);
  p+= sprintf (p, "state: %d  gradient: %d  %s\n", 
	       mydata->state, mydata->gradient, (mydata->WG==WAIT?"WAIT":"GO"));

  p += sprintf (p, "%d neighbors: ", mydata->N_Neighbors);

  for (i = 0; i < mydata->N_Neighbors; i++)
    //    p += sprintf (p, "%d ", mydata->neighbors[i].ID);
    p += sprintf (p, "[%d,%d,%d]", mydata->neighbors[i].ID, mydata->neighbors[i].dist, mydata->neighbors[i].gradient);

  
  return botinfo_buffer;
}
#endif 
