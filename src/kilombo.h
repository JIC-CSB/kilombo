#ifndef KILOMBO_H
#define KILOMBO_H


#ifdef __AVR__

#include <kilolib.h>

#else

#include "kilombo/kilolib.h"

#endif

/* Declarations for handling the mydata pointer in
 * the simulator and the real kilobot
 */

#ifdef SIMULATOR 

#include "kilombo/params.h"

// fill in the size of the USERDATA structure,
// used by the simulator to allocate space for it.

#define REGISTER_USERDATA(UDT) 		\
	int UserdataSize = sizeof(UDT); \
	UDT *mydata;

#else // compiling for the real kilobot

// declare one instance of the USERDATA structure,
// make mydata point to it.

#define REGISTER_USERDATA(UDT) 		\
	UDT myuserdata;                 \
	UDT *mydata = &myuserdata; 

#define SET_CALLBACK(ID, CALLBACK)

#endif	// SIMULATOR


#endif	// KILOMBO_H
