#include <kilombo.h>

#include "follow.h"
#include "communication.h"

extern USERDATA * mydata;


// message rx callback function. Pushes message to ring buffer.
void rxbuffer_push(message_t *msg, distance_measurement_t *dist)
{
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

/* Process a received message at the front of the ring buffer.
 * Go through the list of neighbors. If the message is from a bot
 * already in the list, update the information, otherwise
 * add a new entry in the list
 */

// void process_message (message_t *msg, distance_measurement_t *dist)
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
  mydata->neighbors[i].gradient = data[3] | (data[4] << 8);
  mydata->neighbors[i].follower = data[5] | (data[6] << 8);
  mydata->neighbors[i].WG = data[7];
}

/* Go through the list of neighbors, remove entries older than a threshold,
 * currently 1 seconds.
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



void setup_message(void)
{
  mydata->message_lock = 1;  //don't transmit while we are forming the message
  mydata->transmit_msg.type = NORMAL;
  mydata->transmit_msg.data[0] = kilo_uid & 0xff; //0 low  ID
  mydata->transmit_msg.data[1] = kilo_uid >> 8;   //1 high ID
  mydata->transmit_msg.data[2] = mydata->N_Neighbors;     //2 number of neighbors
  
  mydata->transmit_msg.data[3] = mydata->gradient & 0xff;
  mydata->transmit_msg.data[4] = mydata->gradient >> 8;
  
  mydata->transmit_msg.data[5] = mydata->follower & 0xff;
  mydata->transmit_msg.data[6] = mydata->follower >> 8;
  mydata->transmit_msg.data[7] = mydata->WG;
  
  mydata->transmit_msg.crc = message_crc(&mydata->transmit_msg);
  mydata->message_lock = 0;
}


