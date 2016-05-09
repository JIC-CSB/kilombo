# Kilombo - C kilobot simulator

Authors: Fredrik Jansson, Matthew Hartley, Martin Hinsch, 
         Tjelvar Olsson, Ivica Slavkov, Noemí Carranza

For an extended list of those contributing to the simulator and related paper, see [here](contributors.md).

Contact: 
Fredrik Jansson: fjansson@abo.fi
Matthew Hartley: Matthew.Hartley@jic.ac.uk


This repository holds the code for Kilombo, a C based Kilobot
simulator. The simulator was developed in the [Swarm Organ
project](http://www.swarm-organ.eu), and is distributed under the MIT license
(see the file LICENSE for details). Installation instructions follow
below. For detailed usage instruction and programming documentation,
see `docs/manual.md` and the example bots in `examples/`.

A preprint of an article about the simultator is available on
arXiv. If the Kilombo simulator is used for academic purposes, we
would highly appreciate citations of this article and the GitHub
repository.  [ArXiv 1511.04285](http://arxiv.org/abs/1511.04285)

Any feedback on the simulator is welcome, contact the authors listed
above. If the simulator turns out to be useful for you, we would like
to hear it - please write us a short mail to let us know.


## Prerequisites
You'll need to have:
- git
- cmake
- SDL 1.X (including headers for building, i.e. devel packages under linux)
- the [Jansson library](http://www.digip.org/jansson/) (though it's in
  many package managers. Again, get the development package)
- the [Check unit testing library](http://check.sourceforge.net/)

Additionally, to compile the code for real kilobots as well, the following are needed:
- avr-gcc (c compiler for the AVR microcontroller, to compile for the kilobot)
- [Kilolib](https://github.com/acornejo/kilolib) (the official Kilobot library)


### Debian-based Linux systems
`sudo apt-get install build-essential git gcc-avr gdb-avr binutils-avr
avr-libc avrdude libsdl1.2-dev libjansson-dev cmake check`

### OSX systems

Standard development tools, includeing git and a c compiler, can be
obtained by installing xcode.  In addition, several libraries are
needed, which can be obtained using the package manager brew.  A
tutorial:
http://hackercodex.com/guide/mac-osx-mavericks-10.9-configuration/

With brew installed, install the libraries.

cmake:
`brew install cmake`

check library:
`brew install check`

SDL library:
`brew install sdl`

AVR toolchain, (Optional, needed for real kilobots):
`brew tap osx-cross/avr`
`brew install avr-libc`

For the brew-installed libraries to be found, the following lines can be
added to `.bash_profile`:
`export C_INCLUDE_PATH=/usr/local/include/:$C_INCLUDE_PATH`
`export LIBRARY_PATH=/usr/local/lib:$LIBRARY_PATH`


## Build instructions

Make a build directory:

	mkdir build
	cd build

Run cmake:

	cmake ..

Install:

	sudo make install

This installs the kilobot simulator in a system-wide location, by default
under `/usr/local/lib` and `/usr/local/include`.


## Compile and run examples
cd examples
cd orbit
make     # or make -f Makefile.osx on OSX
./orbit -b start_positions.json

The example makefiles are set up to build the code for the simulator by default. By running
    make hex
the same source code is compiled for the real Kilobot. This requires the avr-gcc toolchain
and the official Kilolib to be installed (see prerequisites above). 

## Setup for compiling programs for the real Kilobots

### Kilolib

    git clone https://github.com/acornejo/kilolib
    cd kilolib
    make

add `export KILOHEADERS=/path/to/kilolib/` to `.bash_profile` or another suitable
configureation file, or provide the path in the Makefile for the bot program.
For the setting to take effect, open a new terminal or `source .bash_profile`. 

To compile the example programs for real robots:
`cd examples/orbit`  # or another example 
`make hex`


## Third-party code
The simulator uses the [SDL_GFX library](http://cms.ferzkopp.net/index.php/software/13-sdl-gfx)
by Andreas Schiffler (distributed under the zlib license). The source
code for the sdl_gfx library is bundled with the simulator due to its
unreliable availability in various Linux distributions. It is located in the
directory src/gfx, and is automatically compiled with the rest of the
source code.

The simulator also includes the API definitions of kilolib, the C
library for the physical kilobot robot. These are located in the file
src/kilolib.h, with minor, simulator-specific changes.
The full kilolib source code is here: https://github.com/acornejo/kilolib .
