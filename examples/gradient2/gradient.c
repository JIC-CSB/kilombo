/* The gradient demonstration from the kilobotics-labs
 * https://www.kilobotics.com/labs#lab6-gradient
 * 
 * the example in examples/gradient/  is close to the kilobotics lab implementation.
 * this version,  examples/gradient2/ maintains a table of neighbors, and uses that to 
 * calculate the current value - which means that the value is dynamically updated if the 
 * robots are moved around.
 * 
 *
 * The robot with ID 0 is initialized ith the gradient_value 0
 * Every other bot gets the smallest-value-of-current-neighbors + 1 as its own value
 * 
 * In the simulator one can force a reset by pressing F6, which calls setup() 
 * using the callback mechanism.
 *
 * Lightly modified to work in the simulator, in particular:
 *  - mydata->variable for global variables
 *  - callback function botinfo() to report bot state back to the simulator for display
 *  - callback function json_state() to save bot state as json
 *  - setup() is used as reset callback function 
 *  - we use a table of 10 rainbow colors instead of just 3 colors. 
 *  - use of a neighbor table
 *
 *
 * Modifications by Fredrik Jansson 2015
 */

#include <kilombo.h>

#include "gradient.h"  // defines the USERDATA structure


#define CUTOFF 100 //neighbors further away are ignored. (mm)

REGISTER_USERDATA(USERDATA)


// rainbow colors
uint8_t colors[] = {
  RGB(0,0,0),  //0 - off
  RGB(2,0,0),  //1 - red
  RGB(2,1,0),  //2 - orange
  RGB(2,2,0),  //3 - yellow
  RGB(1,2,0),  //4 - yellowish green
  RGB(0,2,0),  //5 - green
  RGB(0,1,1),  //6 - cyan
  RGB(0,0,1),  //7 - blue
  RGB(1,0,1),  //8 - purple
  RGB(3,3,3)   //9  - bright white
};


// message rx callback function. Pushes message to ring buffer.
void rxbuffer_push(message_t *msg, distance_measurement_t *dist) {
    received_message_t *rmsg = &RB_back();
    rmsg->msg = *msg;
    rmsg->dist = *dist;
    RB_pushback();
}

void setup_message(void)
{
  mydata->msg.type = NORMAL;
  mydata->msg.data[0] = kilo_uid & 0xff;         // 0 low  ID
  mydata->msg.data[1] = kilo_uid >> 8;           // 1 high ID
  mydata->msg.data[2] = mydata->N_Neighbors;     // 2 number of neighbors


  mydata->msg.data[3] = mydata->gradient_value&0xFF;      // 3 low  byte of gradient value
  mydata->msg.data[4] = (mydata->gradient_value>>8)&0xFF; // 4 high byte of gradient value
    
  mydata->msg.crc = message_crc(&mydata->msg);
}


message_t *message_tx() {
    if (mydata->gradient_value != UINT16_MAX)
        return &mydata->msg;
    else
        return '\0';
}


/* Process a received message at the front of the ring buffer.
 * Go through the list of neighbors. If the message is from a bot
 * already in the list, update the information, otherwise
 * add a new entry in the list
 */

void process_message () 
{
  uint8_t i;
  uint16_t ID;

  uint8_t d = estimate_distance(&RB_front().dist);
  if (d > CUTOFF)
    return;

  uint8_t *data = RB_front().msg.data;
  ID = data[0] | (data[1] << 8);


  // search the neighbor list by ID
  for (i = 0; i < mydata->N_Neighbors; i++)
    if (mydata->neighbors[i].ID == ID) // found it
      break;

  if (i == mydata->N_Neighbors)   // this neighbor is not in list
    if (mydata->N_Neighbors < MAXN-1) // if we have too many neighbors, we overwrite the last entry
      mydata->N_Neighbors++;          // sloppy but better than overflow

  // i now points to where this message should be stored
  mydata->neighbors[i].ID = ID;
  mydata->neighbors[i].timestamp = kilo_ticks;
  mydata->neighbors[i].dist = d;
  mydata->neighbors[i].N_Neighbors = data[2];
  mydata->neighbors[i].gradient = data[3] | data[4]<<8;

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
	mydata->neighbors[i] = mydata->neighbors[mydata->N_Neighbors-1]; //replace it by the last entry
	mydata->N_Neighbors--;
      }
}


void setup() {
  mydata->gradient_value = UINT16_MAX;

  mydata->N_Neighbors = 0;

  setup_message();
  // the special root bot originally had UID 10000, we change it to 0
  if (kilo_uid == 0)  
        mydata->gradient_value = 0;

  setup_message();
}

void loop()
{
  purgeNeighbors();

  //process messages in the RX ring buffer
  while (!RB_empty()) {
    process_message();
    RB_popfront();
  }

  uint8_t i;
  uint16_t min = UINT16_MAX-1;

  for (i = 0; i < mydata->N_Neighbors; i++)
    if (mydata->neighbors[i].gradient < min)
      min = mydata->neighbors[i].gradient;

  if (kilo_uid == 0)  
        min = 0;
  
  if (mydata->gradient_value != min+1)
    {
      mydata->gradient_value = min+1;
      setup_message();
    }
  
  set_color(colors[mydata->gradient_value%10]);
}

#ifdef SIMULATOR
/* provide a text string for the simulator status bar about this bot */
static char botinfo_buffer[10000];
char *botinfo(void)
{
  int i;
  char *p = botinfo_buffer;
  p += sprintf (p, "ID: %d \n", kilo_uid);
  p += sprintf (p, "Gradient Value: %d\n", mydata->gradient_value);

  p += sprintf (p, "%d neighbors: ", mydata->N_Neighbors);

  for (i = 0; i < mydata->N_Neighbors; i++)
    p += sprintf (p, "%d ", mydata->neighbors[i].ID);
  
  return botinfo_buffer;
}

#include <jansson.h>
json_t *json_state();
#endif

int main(void) {
    // initialize hardware
    kilo_init();

    // initialize ring buffer
    RB_init();

    // register message callbacks
    kilo_message_rx = rxbuffer_push; 
    kilo_message_tx = message_tx;   

    // register your program
    kilo_start(setup, loop);

    SET_CALLBACK(botinfo, botinfo);
    SET_CALLBACK(reset, setup);
    SET_CALLBACK(json_state, json_state);
    
    
    return 0;
}
