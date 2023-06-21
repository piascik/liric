# Nudgematic library

This library is used to control 'nudgematic' mechanism on LIRIC. This mechanism allows for the offseting of the pointing centre on the detector, without moving the telescope (which is slow and takes time to settle). It consists of two platform mounted on rails (horizontal and vertical). Both platforms can be moved by turning a motor attached to a cam, with odd-sized lobes giving two different sizes of offset in each direction. There is a resistor based encoder attached to the motor output shaft that tells us the position of each cam. The motor and encoder are attached to an Arduino Mega and motor shield, which can turn the motors on and off, and read the encoders.

## Directory structure

* **c** The source code for the library.
* **include** The header files/API of the library.
* **test** The command line test programs for the library.

