# Introduction
This tool is a simulator for kilobot robots. The kilobot c program is compiled natively on the host computer where the simulator is to be run. The robot program is linked with a simulator library, which provides the functions from the kiloblib API. Since the robot program is compiled to native code, the simulator is very fast. 

The simulator uses SDL for grapical output. All configuration and result files are written in JSON.

The program is distributed under the *WHICH LINCESE??* license, with no warranty.
The simulator was developed in the (SWARM Organ)[www.swarm-organ.eu] project.

Contact: 
- Fredrik Jansson fjansson@abo.fi
- Matthew Hartley 


# Installation
Installation instructions are found in the file `README.md` in the root directory of the project.

# Adapting code
The simulator aims to be compatible with the [kilolib API.](https://www.kilobotics.com/docs/index.html)

Initialization, messaging, motion, and random number generation 
work with the same function calls in the simulator and in the
real kilobot. Two topics that require special care when using the simulator are 
global variables and timing.

## Variables
Kilobot C code usually makes use of static variables (to allow these
to persist across repeated calls to the user supplied function) or
global variables.  These variables demand special treatment when run
in the simulator.  The simulator handles all bots in a single program,
so a global or static variable ends up being common to all bots.

A workaround implemented in the simulator is to keep all global variables
inside a `struct`, defined in the bot's header file. 

    typedef struct 
    {
        int N_Neighbors;
        ...
    } USERDATA;

In the program, the variables can then be accessed using `mydata->N_Neighbors`.
The simulator ensures that `mydata` points to the data of the currently
running bot. Note that local variables (i.e. regular variables defined
inside a function) can be used in the usual way. 

When the program is compiled for a real kilobot, the same way of accessing
the variables can be used, provided that the program declares one instance of the `USERDATA` structure. 

    #ifdef KILOBOT
      USERDATA myuserdata;
      USERDATA *mydata = &myuserdata;
    #endif


## Timing and delays 
The simulator does not implement the `delay()` function at all, since
it would be difficult.  The delay() function exists, but returns immediately. The simulator simply calls the bot's main loop  function  once every simulator time step, for every bot. The main loop function is the one specified when calling `kilo_init()`.

Note that in programs for real kilobots, the kilolib API documentation states that it is best to use `delay()` only for short times, like when spinning
up motors, and that  for timing the bot's behaviour, one should instead use the global variable `kilo_ticks`. `kilo_ticks` is incremented 31 times per second, and is  implemented in the simulator as well.


## Callback functions
For convenience, the simulator implements a system of callback functions for communication with the user-specified robot program. 


# Controls

The following keybindings are active during simulation:

* ESC: terminate simulation
* Numpad +: zoom in
* Numpad -: zoom out
* Arrow keys : move display
* Space : pause/resume simulation
* Mouse-left: drag bots around
* Mouse-right: rotate a bot
* s   : Take screenshot. The screenshot is stored as screenshots/<bot name>.bmp
* F1  : disperse the bots
* F2  : compress the bots
* F5  : Call the bot's parameter reload function (if implemented in the bot)
* F6  : Call every bot's reset function, (if implemented in the bot)
* F11 : Toggle full speed simulation (no delay between frames)
* F12 : Toggle fast communication (message passing every kilotick)

# Configuration file settings

The following keywords are recognized in the simulator JSON configuration file:

* `botName` : the name of this bot type
* `randSeed` : a random seed, for repeatable simulations
* `nBots` : number of bots to simulate     
* `timeStep` : time step. Also determines the displayed frame rate (currently, will be configurable)
* `simulationTime` : how long to run the simulation
* `showComms` :  whether or not to draw a line between each pair of bots in communication range, whenever a message is passed between them.
* `showCommsRadius` :  whether or not to draw a circle for the communications range of each bot
* `distributePercent` :
* `displayWidth`  : absolute width of the window, pixels
* `displayHeight` : absolute height of the window, pixels
* `showHist` : whether to show the paths the robots have moved
* `imageName` :  File name for storing images during the simulation. Format example: `movie/f%04d.bmp`,
  where the `%...d` will be replaced by an increasing number.

#Command line options
* `-p parameterfile.json` : Simulator parameters. Mandatory
* `-b bots.json` : starting positions for the bots. Optional.

At the end of the simulation, the simulator stores the final state of the bots in a file named `endstate.json`. This file can be given as a starting state for the next simulation, simply copy it to a new name, and pass that name to the simulator with the -b option. Thus the simulator can be used as an editor of bot starting configurations as well.


