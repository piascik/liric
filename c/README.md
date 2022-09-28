# C layer

This directory contains the source code for the 'raptor' C layer. This program exposes a telnet style interface
that allows commands to be sent to it (from the Java layer) and replies returned. The software controls the following:

* Raptor Ninox-640 detector
* Nudgematic mechansism (instrument imaging offseting mechanism) via a USB-PIO I/O board.
* Starlight Express filter wheel.

## Directory structure

This directory contains the source code , Makefile and configuration property files. The header files for each module
are in the 'include' directory in the main raptor repository (i.e. at the same level as this directory).
