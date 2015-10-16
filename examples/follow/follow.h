#ifndef FOLLOW_H
#define FOLLOW_H

#include <stdio.h>     // for printf
#include <math.h>


#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944
#endif


#define MAXN 22
#define RB_SIZE 16   // Ring buffer size. Choose a power of two for faster code

typedef struct {
    message_t msg;
    distance_measurement_t dist;
} received_message_t;

typedef struct {
  uint16_t ID;
  uint8_t dist;
  uint8_t N_Neighbors;
  uint32_t timestamp;
  uint16_t gradient;
  uint16_t follower;
  uint8_t WG;
} Neighbor_t;



#define WAIT 0
#define GO 1

typedef struct 
{
  Neighbor_t neighbors[MAXN];
  uint8_t N_Neighbors;
  message_t transmit_msg;
  char message_lock;
  uint32_t next_ticks;
  uint32_t next_movement_ticks;
  received_message_t RXBuffer[RB_SIZE];
  // uint8_t RXHead, RXCount;
  uint8_t RXHead, RXTail;

  uint8_t turn;
  uint8_t badsteps;
  uint16_t gradient;
  uint16_t cycle_time;
  uint16_t movement_time;

  uint16_t follower;

  uint8_t WG;
  uint8_t prevWG;
  
  float Vmin;
  uint8_t changeTicks;
  
  uint8_t state;
  
} USERDATA;


// Ring buffer operations. Taken from kilolib's ringbuffer.h
// but adapted for use with mydata->

// Ring buffer operations indexed with head, tail
// These waste one entry in the buffer, but are interrupt safe:
//   * head is changed only in popfront
//   * tail is changed only in pushback
//   * RB_popfront() is to be called AFTER the data in RB_front() has been used
//   * head and tail indices are uint8_t, which can be updated atomically
//     - still, the updates need to be atomic, especially in RB_popfront()

#define RB_init() {	\
    mydata->RXHead = 0; \
    mydata->RXTail = 0;\
}

#define RB_empty() (mydata->RXHead == mydata->RXTail)

#define RB_full()  ((mydata->RXHead+1)%RB_SIZE == mydata->RXTail)

#define RB_front() mydata->RXBuffer[mydata->RXHead]

#define RB_back() mydata->RXBuffer[mydata->RXTail]

#define RB_popfront() mydata->RXHead = (mydata->RXHead+1)%RB_SIZE;

#define RB_pushback() {\
    mydata->RXTail = (mydata->RXTail+1)%RB_SIZE;\
    if (RB_empty())\
      { mydata->RXHead = (mydata->RXHead+1)%RB_SIZE;	\
	printf("Full.\n"); }				\
  }


/*
// Ring buffer operations indexed with head, count
// These save one entry in the buffer, but are less interrupt safe due to the shared count

#define RB_init() {	\
    mydata->RXHead = 0; \
    mydata->RXCount = 0;\
}
#define RB_empty() (mydata->RXCount == 0)
#define RB_full()  (mydata->RXCount == RB_SIZE)
#define RB_front() (mydata->RXBuffer[mydata->RXHead])
#define RB_back()  (mydata->RXBuffer[(mydata->RXHead+mydata->RXCount)%RB_SIZE])
#define RB_popfront() {\
    mydata->RXHead = (mydata->RXHead+1)%RB_SIZE;\
    mydata->RXCount--;\
}

#define RB_pushback() {\
    if (RB_full())\
        mydata->RXHead = (mydata->RXHead+1)%RB_SIZE;\
    else\
      mydata->RXCount++;\
}
*/


#endif
