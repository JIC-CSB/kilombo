# ckbsim - C kilobot simulator

This repository holds the code for ckbsim, a C based kilobot simulator.
Installation instructions follow below. For detailed usage instruction and programming documentation, see `docs/manual.md` and the example bots in `examples/`.

## Prerequisits
You'll need to have:
- git
- cmake
- avr-gcc (c compiler for the AVR microcontroller, to compile for the kilobot)
- SDL 1.X (including headers for building, i.e. devel packages under linux)
- the [Jansson library](http://www.digip.org/jansson/) (though it's in many package managers. Again, get the development package)


### Debian-based Linux systems
`sudo apt-get install build-essential git gcc-avr gdb-avr binutils-avr avr-libc avrdude libsdl1.2-dev libjansson-dev cmake `


## Build instructions

Make a build directory:

	mkdir build
	cd build

Run cmake:

	cmake ..

Install:

	make install

This installs the kilobot simulator in a system-wide location, by default
under `/usr/local/lib` and `/usr/local/include`.
