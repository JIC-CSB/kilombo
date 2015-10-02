/* Declarations for handling the mydata pointer in
 * the simulator and the real kilobot
 */

#ifdef SIMULATOR 

// fill in the size of the USERDATA structure,
// used by the simulator to allocate space for it.

int UserdataSize = sizeof(USERDATA);
USERDATA *mydata;

#else // compiling for the real kilobot

// declare one instance of the USERDATA structure,
// make mydata point to it.

USERDATA myuserdata;             
USERDATA *mydata = &myuserdata;

#endif
