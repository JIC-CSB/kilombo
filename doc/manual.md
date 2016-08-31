# Introduction
This tool is a simulator for kilobot robots. The kilobot c program is compiled natively on the host computer where the simulator is to be run. The robot program is linked with a simulator library, which provides the functions from the kiloblib API. Since the robot program is compiled to native code, the simulator is very fast. The simulator uses SDL for grapical output. All configuration and result files are written in JSON.

The program is distributed under the MIT license, with no warranty, please see the file LICENSE for details. The simulator was developed in the [SWARM Organ](www.swarm-organ.eu) European Union FP7 project.

Contact: 
- Fredrik Jansson: fjansson@abo.fi
- Matthew Hartley: Matthew.Hartley@jic.ac.uk

A preprint of an article about the simultator is available on
arXiv. If the Kilombo simulator is used for academic purposes, we
would highly appreciate citations of this article and the GitHub
repository.  [ArXiv 1511.04285](http://arxiv.org/abs/1511.04285)

Any feedback on the simulator is welcome, contact the authors listed
above. If the simulator turns out to be useful for you, we would like
to hear it - please write us a short mail to let us know.


# Installation
Installation instructions are found in the file `README.md` in the root directory of the project.

# Adapting code
The simulator aims to be compatible with the [kilolib API.](https://www.kilobotics.com/docs/index.html)

Initialization, messaging, motion, and random number generation work with the same function calls in the simulator and in the real kilobot. Two topics that require special care when using the simulator are global variables and timing.

A program for the Kilombo simulator should begin by including the file kilombo.h, which is distributed with the simulator. The program does not need to include kilolib.h, since kilombo.h handles this.

    #include <kilombo.h>


## Variables
Kilobot C code usually makes use of static variables (to allow these to persist across repeated calls to the user supplied function) or global variables.  These variables demand special treatment when run in the simulator.  The simulator handles all robots in a single program, so a global or static variable ends up being common to all robots. A workaround implemented in the simulator is to keep all global variables inside a single `struct`. The type of this user data has to be announced to the simulator by using the macro `REGISTER_USERDATA`.


    typedef struct 
    {
        int N_Neighbors;
        ...
    } USERDATA;

    REGISTER_USERDATA(USERDATA)

In the program, the variables can then be accessed using e.g. `mydata->N_Neighbors`. The simulator ensures that `mydata` points to the data of the currently running bot. Note that local variables (i.e. regular variables defined inside a function) can be used in the usual way. 

### A note on the implementation:

The `REGISTER_USERDATA` macro resolves to (a.o.) a declaration `USERDATA *mydata`. The simulator ensures that this pointer points to the data of the correct robot before calling any of the user program's functions. Additionally, the simulator needs to know the size of the `USERDATA` structure (however, it does not need to know the contents of it, and it will never change it). The size of the structure is communicated to the simulator by declaring a variable `UserdataSize`, and setting it to the size of the structure. When compiling for the simulator the macro call above is therefore roughly equivalent to the following code:

    #ifdef SIMULATOR
    int UserdataSize = sizeof(USERDATA);
    USERDATA *mydata;
    #endif


## Timing and delays 
The simulator does not implement the `delay()` function at all, since it would be difficult.  The delay() function exists, but returns immediately. The simulator simply calls the bot's main loop function once every simulator time step, for every bot. The main loop function is the one specified when calling `kilo_init()`.

Note that in programs for real kilobots, the kilolib API documentation states that it is best to use `delay()` only for short times, like when spinning up motors, and that  for timing the bot's behaviour, one should instead use the global variable `kilo_ticks`. `kilo_ticks` is incremented 31 times per second, and is implemented in the simulator as well.


## Data types
A difference between the AVR c compiler used for the kilobots and the native c compiler used when compiling with the simulator is the size of datatypes. For example, `int` is 16 bits on the AVR and 32 bits on a standard 32 or 64 bit PC. This should normally not be a problem, unless integer overflow is used on purpose. However it may lead to code working as intended in the simulator while overflowing on the kilobot.
A solution is to explicitly specify the size of the types, e.g. declaring variables as `uint8_t i`. This is good practice on the AVR anyway, since it lets the programmer conserve RAM memory by using the smallest possible type.


## Callback functions
For convenience, the simulator implements a system of callback functions for communication with the user-specified robot program. The callbacks are registered using the macro `SET_CALLBACK(ID, function)`, typically at the beginning of `main()` before calling `kilo_init()`. All callback functions are optional. If the bot does not register them, they are not used. Note also that the macro resolves to an empty statement if the code is compiled for the kilobot platform.

|ID  | example function definition  |  use | 
| -------------|-----------------------------------|--------|
|`params`      | `void  callback_F5(void)`         | Reload configuration parameters from file. Called once, not for every bot, when F5 is pressed.|
|`reset`       | `void  callback_F6(void)`         | Reset bot. Called for every bot, when F6 is pressed. |
|`botinfo`     | `char *botinfo()`                 | Return a string describing the internal state of the current bot, used for the simulator status bar.|
|`json_state`  | `json_t* json_state(void)`        | Return a json object describing the bot's internal state. Used to store snapshots of the simulation. |
|`global_setup`| `void callback_global_setup(void)`| Perform global setup, such as reading additional simulation-specific parameters. Called once, after the parameter file has been read but before the bot-specific setup.|
| `lighting`   | `int16_t callback_lighting(double, double)`                  | Set user-defined light levels.       | 
| `obstacles`  | `int callback_obstacles(double, double, double *, double *)` | Set user-defined physical obstacles. |


The functions need to be defined with return types as specified in the table above. For an example of the botinfo callback, see `examples/orbit/orbit.c`.
For an example of the json state saving callback, see `examples/gradient/gradient.c`.

In many situation it is desirable to let bots interact with their environment. For simple cases the simulator provides facilities to let users define a lighting regime as well as physical obstacles without having to change the code of the simulator itself.


### Lighting

Using the callback ID `lighting` a callback function can be set that calculates light levels from x,y coordinates. Important note: In order to stay as close as possible to the physical limitations of the real kilobots the result of the callback function will be truncated to the interval [0,1023] before supplying the value to the bots.

### Obstacles

In a similar way obstacles can be defined by setting a callback function using `obstacles`. The user-supplied function receives x,y coordinates and pointers to x,y delta values. It has to return 0 if *no* obstacle is present at the coordinates and any other value otherwise. The motion that results from colliding with the obstacle is provided by setting the second set of coordinates.

# Controls

The following keybindings are active during simulation:

* ESC: terminate simulation
* + : zoom in
* - : zoom out
* Numpad * or F4: faster
* Numpad / or F3: slower
* Arrow keys : move display
* Space : pause/resume simulation
* Mouse-left: drag bots around
* Mouse-right: rotate a bot
* s   : Take screenshot. The screenshot is stored as screenshots/<bot name><number>.bmp
* v   : start storing screenshots for video, using "imageName" from the simulator parameters as file name, see below.
* F1  : disperse the bots
* F2  : compress the bots
* F5  : Call the bot's parameter reload function (if implemented in the bot)
* F6  : Call every bot's reset function, (if implemented in the bot)
* F11 : Toggle full speed simulation (no delay between frames)
* F12 : Toggle fast communication (message passing every kilotick)

# Configuration file settings and commandline parameters

The following options can be set in the simulator JSON configuration file or on the commandline respectively:

|parameter name  |type  |default value |use  | 
|----------------|------|--------------|-----|
|**Simulation**||||
|`botName`              |string| "default" | the name of this bot type|
|`randSeed`             |int   | 0 | a random seed, for repeatable simulations|
|`simulationTime`       |float | 0 | how long to run the simulation|
|`timeStep`             |float |0.02| Simulator time step. |
|**Scenario**||||
|`nBots`                |int   |*required*| number of bots to simulate     |
|`formation`            |option|`random`| starting bot formation. Current options are (`random`, `line`, `pile`, `circle`, `ellipse`).|
|`distributePercent`    |float |0.2| initially distribute the bots over this fraction of the display width|
|`commsRadius`          |int   |70| the communication range of the robots in mm|
| `msgSuccessRate`    |float |1.0| probability of messages between robots to be transmitted successfully|
| `distanceNoise` 		|float |0| stochasticity of distance measurements (standard deviation)|
| `distanceCoefficient` 	|float |1| slope of bot-bot distance function| 
| `speed` 		|float |7| robot movement speed in mm/s. |
| `turnRate` 		|float |13| robot turning speed in degrees/s. |
| `pushDisplacement` 	|float |1.0| displacement of stationary bots due to pushing |
|**User interface**||||
|`displayWidthPercent`  |float |0.9| if no absolute window size is given use this proportion of the screen width |
|`displayHeightPercent` |float |0.9| if no absolute window size is given use this proportion of the screen height |
|`displayWidth`         |int   |1| absolute width of the window in pixels|
| `displayHeight`       |int   |1| absolute height of the window in pixels|
| `displayScale`        |float |1.0| initial zoom setting.|
| `displayX`            |float |0.0| initial center coordinates of view.|
| `displayY`            |float |0.0| initial center coordinates of view.|
| `showHist`            |int   |0| whether to show the paths the robots have moved|
| `histLength`          |int   |2000| the length of the path history to show in number of steps |
| `showComms`           |int   |1|  whether or not to draw a line between each pair of bots in communication range, whenever a message is passed between them.|
| `showCommsRadius`     |int   |1|  whether or not to draw a circle for the communications range of each bot|
| `stepsPerFrame`       |int   |1| number of simulator time steps to perform between drawing. Can be changed interactively using numpad `/` and `*`. |
| `GUI`                 |int   |1| 0 or 1. If 0, the simulator is run as fast as possible, without displaying the progress. Periodic screenshots can still be stored.|
| `colorscheme`         |option|`dark`| `dark` or `bright`, different color schemes. Bright tends to look better in print.|
|**Data output**||||
| `saveVideo`           |int   |0| 0 or 1, whether or not to store video frames periodically. Can be toggled during simulation by pressing `v`.|
| `saveVideoN`          |int   |1| store every N:th simulation step while saving video.|
| `imageName`           |string|""|  file name for storing images during the simulation. Format example: `movie/f%04d.bmp`, where the `%...d` will be replaced by an increasing number.|
| `finalImage`          |string|""| file name for saving an image of the final simulation state. `null` can be specified to disable this. |
| `storeHistory`        |int   |1| TBD.|
| `stateFileName`       |string|""| file name for saving the simulation state as JSON during the simulation.|
| `stateFileSteps`      |int   |100| number of simulator timesteps between storing the simulator state as JSON. Use 0 to disable storage. |
|**Optimization**||||
| `useGrid` 		|int |1| Whether to use the grid cache to find neighbors. Faster for large swarms (n > 50 robots) |


|**Command line options**|||
|`-p parameterfile.json`|string|<sim name\>.json| Simulator parameters. Optional. |
|`-b bots.json`         |string|""| starting positions for the bots. Optional.|



At the end of the simulation, the simulator stores the final state of the robots in a file named `endstate.json`. This file can be given as a starting state for the next simulation, simply copy it to a new name, and pass that name to the simulator with the -b option. Thus the simulator can be used as an editor of bot starting configurations as well.

#Saving state
At the end of the simulation, and optionally also during the simulation the simulator saves the state of the swarm as JSON.
`endstate.json` contains the final state. For saving the state periodically during the simulation, use the parameters `stateFileName` and `stateFileSteps`.

The json object contains an array named `bot_states`.
Each element in this array contains the data for one bot, with the following keys:

|   key     | value  | 
|-----------|--------|
|ID         | the bot's unique ID number                 |
|direction  | angle in which the bot is poiting, radians |
|x_position | x coordinate, in mm                        |
|y_position | y coordinate, in mm                        |
| state     | a json object describing the internal state of the bot, optionally provided by the callback function `json_state` |




#Example bots
A few example bots are provided with the simulator. They are found in the directory 'examples/'. Some of them are based on examples from www.kilobotics.com, but modified to work on the simulator.

## orbit
One stationary bot, with ID 0, emits messages. Another bot moves and tries to keep a constant distance to the stationary bot, by alternating between turning left and right.

## gradient
The robot with ID 0 is initialized ith the gradient_value 0.  Every other bot gets the smallest-value-ever-received + 1 as its own value.  Note that the gradient_value never increases, even if the bots are moved around.  In the simulator one can force a reset by pressing F6, which calls setup()  using the callback mechanism.
Callbacks in use: `reset`, `botinfo`, `json_state`.

## gradient2
This version of the gradient example maintains a table of neighbors, and uses that to  calculate the current gradient_value.  This means that the value is dynamically updated if the  robots are moved around. The robot with ID 0 is initialized with the gradient_value 0.   Every other bot gets the smallest-value-of-current-neighbors + 1 as its own value  In the simulator one can force a reset by pressing F6, which calls `setup()` using the callback mechanism.

A side effect of the dynamic update is that if the root bot is removed from the swarm, the other robots will update their gradient_value in a wave-like manner, to higher and higher values.
 


# Implementation decisions
The following are some justifications for the way the simulator was implemented, and some thoughts on other ways to do it.

There is already a kilobot simulator implemented in Python, kbsim by Antti Halme, https://github.com/ajhalme/kbsim .  This simulator implements a physical model of the kilobots, with motion, collisions and messaging between neighbors. However the program controlling the robots in the simulator is written in Python, making the programming significantly different from programming actual kilobots.

We wanted the simulator to run the same program as the actual kilobots, to make the simulator useful not only for trying out an algorithm in principle but also for testing an actual kilobot implementation. 

* A model of the physical world, where the kilobots have a position, a direction, and can move.
* An implementation of the kilolib API, coupled to the physical model
    * motion
    * LED color
    * communication
    * sensor input
* A way to run the programs of all robots in parallel, and synchronizedly

The kilobot API specifies that the user program should be written as a function, which is called repeatedly by the kilobot library as long as the robot is in the RUNNING state. 
A simple way to simulate many robots in parallel is to sequentially execute the loop function for each robot. This means that the robot program should be written in a way that is independent of how long it takes to execute the loop function. Also, it means a delay in the middle of the loop function is difficult to implement. We found that these limitations are possible to deal with in practice. First, timed events should be implemented using timers, not using delays. The kilobot API defines a varible `kilo_ticks`, which is incremented 31 times a second. The simulator implements `kilo_ticks` as well. This variable can be used for measuring times and for waiting for specific lengths of time. In the simulator, it is possible to control how many iterations of the loop function are run, before the `kilo_ticks` variable is incremented. This implementation means that the time it takes to run the loop function is not accurately simulated. Effectively, the simulation assumes that the loop function runs instantly. 

In order to simulate delays in the middle of the loop function, it would be possible to run the programs for all robots in parallel as separate threads. The threads would then need to be synchronized with each other and with the physical simulation, e.g. using barriers. We decided that for our application, this level of simulation is not needed, and therefore we did not pursue this option further. If the robots are running as separate threads the program could take advantage of multiple processor cores, but on the other hand the synchronization of the threads would introduce some overhead as well.

On the extreme end of the realism spectrum, it would be possible to run the same microcontroller program as the kilobot runs in an AVR simulator, such as `simavr`. Every robot would run in one instance of the AVR simulator, and all these instances would be coupled to a physical model of the world where the kilobots are present. This would however require a detailed physical model of the kilobot, on the level of how individual IO pins on the AVR control the motors etc.  For messaging to work, the separte AVR simulator instances would need to be tightly synchronized. This approach seems possible in principle, but was deemed to be too complicated to implement. It would result in a more accurate simulation, free of the restrictions on delay functions and the `mydata->` access to variables, but would also introduce a significant overhead in simulating the AVR microcontroller instead of compiling the robot C-language program to native machine code.






