# ckbsim - C kilobot simulator

This repository holds the code for ckbsim, a C based kilobot simulator.
Installation instructions follow below. For detailed usage instruction and programming documentation, see `docs/manual.md` and the example bots in `examples/`.

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
