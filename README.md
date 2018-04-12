# Scorgasmatron

This is the code for a hardware device that makes it easy to keep score in table tennis games with 21-point or 11-point games. It is written entirely in c++14 and is targeted toward the atmega328p microcontroller, but could readily be ported to other microcontrollers.

# Images

![front](https://raw.githubusercontent.com/jonesinator/scorgasmatron/master/front.jpg)

![back](https://raw.githubusercontent.com/jonesinator/scorgasmatron/master/back.jpg)

# Usage

The avr-gcc toolchain must be installed. Once the toolchain is installed simply use the `make` command to create a hex file that can be programmed onto the microcontroller.

The `make program` command can be used to program the microcontroller assuming a usbtiny-based programmer is installed. I am using the Sparkfun Pocket AVR Programmer.

The `make clean` command can be used to remove any generated files from the make process.

# Files

* avr\_io.hpp - Header-only library containing abstractions for AVR microcontrollers. Contains low-level classes for setting up pin assignments as input or output, and contains high-level classes for software debounced buttons and seven segment displays. This may eventually be pulled into its own repository if it proves to be reusable enough.
* table\_tennis.hpp - Header-only library encapsulating all logic for games of table tennis. This is generic and could be used for any application, it has no microcontroller-specific code in it.
* scorgasmatron.cpp - The main driver. Contains pin definitions (all pins are used), and contains the main program loop that interacts with the buttons and displays.
* Makefile - Builds the hex file that can be uploaded to the microcontroller.

# To Do

* Complete hardware schematics and PCB layout.
* Create Vagrantfile to set up a build environment.
* Demo video.
